/*
 * Copyright (C) 2008 by Singularity Institute for Artificial Intelligence
 * Written by Joel Pitt <joel@fruitionnz.com>
 * All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License v3 as
 * published by the Free Software Foundation and including the exceptions
 * at http://opencog.org/wiki/Licenses
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program; if not, write to:
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
#include "ImportanceUpdatingAgent.h"
#include "Config.h"
#include "mt19937ar.h"

#include <math.h>
#include <time.h>
#include <algorithm>

namespace opencog
{

ImportanceUpdatingAgent::ImportanceUpdatingAgent()
{
    // init starting wages/rents. these should quickly change and reach
    // stable values, which adapt to the system dynamics
    STIAtomRent = DEFAULT_ATOM_STI_RENT;
    LTIAtomRent = DEFAULT_ATOM_LTI_RENT;
    STIAtomWage = DEFAULT_ATOM_STI_WAGE;
    LTIAtomWage = DEFAULT_ATOM_LTI_WAGE;

	rentType = RENT_LOG; //config().get_int("RENT_TYPE");
	amnesty = 5; //config().get_int("RENT_AMNESTY");

    updateLinks = true;

    noiseOn = false;
    noiseOdds = 0.20f;
    noiseUnit = 10;

    // set decay rates for dampened values
    totalStimulusSinceReset.decay = 0.5f;
    attentionalFocusSize.decay = 0.8f;
    attentionalFocusNodesSize.decay = 0.8f;

    targetLobeSTI = config().get_int("STARTING_STI_FUNDS");
    acceptableLobeSTIRange[0] = targetLobeSTI - config().get_int("STI_FUNDS_BUFFER");
    acceptableLobeSTIRange[1] = targetLobeSTI + config().get_int("STI_FUNDS_BUFFER");
    targetLobeLTI = config().get_int("STARTING_LTI_FUNDS");
    acceptableLobeLTIRange[0] = targetLobeLTI - config().get_int("LTI_FUNDS_BUFFER");
    acceptableLobeLTIRange[1] = targetLobeLTI + config().get_int("LTI_FUNDS_BUFFER");

    lobeSTIOutOfBounds = false;

    STICap = AttentionValue::MAXSTI / 2;
    LTICap = AttentionValue::MAXLTI / 2;

    initialEstimateMade = false;

    rng = NULL;

    // Provide a logger, but disable it initially
    log = NULL;
    setLogger(new opencog::Logger("ImportanceUpdatingAgent.log", Logger::DEBUG, true));
    log->disable();
}

ImportanceUpdatingAgent::~ImportanceUpdatingAgent()
{
    if (log) delete log;
    if (rng) delete rng;
}

void ImportanceUpdatingAgent::init(CogServer *server)
{
    // Not sure exactly what initial estimates should be made...
    log->fine("ImportanceUpdatingAgent::init");
    initialEstimateMade = true;

	// Perhaps initiate recent_val members to initial
	// size before the mind process begins.
}

void ImportanceUpdatingAgent::setLogger(Logger* log)
{
    if (this->log) delete this->log;
    this->log = log;
    log->fine("Set new logger for ImportanceUpdatingMindAgent");
}

Logger* ImportanceUpdatingAgent::getLogger()
{
    return log;
}

HandleEntry* ImportanceUpdatingAgent::getHandlesToUpdate(AtomSpace *a)
{
    HandleEntry *h;
    if (updateLinks)
        h = a->getAtomTable().getHandleSet(ATOM, true);
    else
        h = a->getAtomTable().getHandleSet(NODE, true);
    return h;
}

void ImportanceUpdatingAgent::run(CogServer *server)
{
    AtomSpace* a = server->getAtomSpace();
    HandleEntry *h, *q;
    AttentionValue::sti_t maxSTISeen = AttentionValue::MINSTI;
    AttentionValue::sti_t minSTISeen = AttentionValue::MAXSTI;

    log->fine("=========== ImportanceUpdating::run =======");
    /* init iterative variables, that can't be calculated in
     * (no pointer to CogServer there) */
    if (!initialEstimateMade) init(server);

    /* Calculate attentional focus sizes */
    updateAttentionalFocusSizes(a);

    /* Random stimulation if on */
    if (noiseOn) {
        log->debug("Random stimulation on, stimulating atoms");
        randomStimulation(a);
    }

    /* Update stimulus totals */
    updateTotalStimulus(a);

    /* Update atoms: Collect rent, pay wages */
    log->debug("Collecting rent and paying wages");

    h = getHandlesToUpdate(a);

    q = h;
    while (q) {
        updateAtomSTI(a, q->handle);
        updateAtomLTI(a, q->handle);

        /* Enfore sti and lti caps */
        enforceSTICap(a, q->handle);
        enforceLTICap(a, q->handle);

        // Greater than max sti seen?
        if (a->getSTI(q->handle) > maxSTISeen) {
            maxSTISeen = a->getSTI(q->handle);
		} else if (a->getSTI(q->handle) < minSTISeen) {
            minSTISeen = a->getSTI(q->handle);
		}

        q = q->next;
    }
    delete h;

    // Update AtomSpace recent maxSTI and recent minSTI 
	if (minSTISeen > maxSTISeen) {
		// if all Atoms have the same STI this will occur
		minSTISeen = maxSTISeen;
	}

    a->updateMaxSTI(maxSTISeen);
    a->updateMinSTI(minSTISeen);
    log->debug("Max STI seen is %d, recentMaxSTI is now %f", maxSTISeen, a->getMaxSTI());
    log->debug("Min STI seen is %d, recentMinSTI is now %f", minSTISeen, a->getMinSTI());

    /* Check AtomSpace funds are within bounds */
    checkAtomSpaceFunds(a);

    if (lobeSTIOutOfBounds) {
        log->debug("Lobe STI was out of bounds, updating STI rent");
        updateSTIRent(a);
    }

    /* Reset Stimulus */
    a->resetStimulus();

}

void ImportanceUpdatingAgent::updateTotalStimulus(AtomSpace* a)
{
    totalStimulusSinceReset.update(a->getTotalStimulus());
}

void ImportanceUpdatingAgent::setNoiseFlag(bool newVal)
{
    noiseOn = newVal;
}

bool ImportanceUpdatingAgent::inRange(long val, long range[2]) const
{
    if (val <= range[1] && val >= range[0])
        return true;
    return false;
}

bool ImportanceUpdatingAgent::checkAtomSpaceFunds(AtomSpace* a)
{
	bool adjustmentMade = false;

    log->debug("Checking STI funds = %d, range=[%d,%d]", a->getSTIFunds(),
               acceptableLobeSTIRange[0], acceptableLobeSTIRange[1]);
    if (!inRange(a->getSTIFunds(), acceptableLobeSTIRange)) {
        log->debug("Lobe STI funds out of bounds, re-adjusting.");
        lobeSTIOutOfBounds = true;
        adjustSTIFunds(a);
		adjustmentMade = true;
    }

    log->debug("Checking LTI funds = %d, range=[%d,%d]", a->getLTIFunds(),
               acceptableLobeLTIRange[0], acceptableLobeLTIRange[1]);
    if (!inRange(a->getLTIFunds(), acceptableLobeLTIRange)) {
        log->debug("Lobe LTI funds out of bounds, re-adjusting.");
        adjustLTIFunds(a);
		adjustmentMade = true;
    }
	return adjustmentMade;
}

opencog::RandGen* ImportanceUpdatingAgent::getRandGen()
{
    if (!rng) {
        rng = new opencog::MT19937RandGen((unsigned long) time(NULL));
    }
    return rng;
}

void ImportanceUpdatingAgent::randomStimulation(AtomSpace* a)
{
    int expectedNum, actualNum;
    HandleEntry *h, *q;
    opencog::RandGen *rng;

    rng = getRandGen();

    // TODO: use util::lazy_random_selector and a binomial dist
    // to get actualNum
    actualNum = 0;

    h = getHandlesToUpdate(a);
    expectedNum = (int) (noiseOdds * h->getSize());

    q = h;
    while (q) {
        double r;
        r = rng->randdouble();
        if (r < noiseOdds) {
            a->stimulateAtom(q->handle, noiseUnit);
            actualNum++;
        }
        q = q->next;
    }

    log->info("Applied stimulation randomly to %d " \
              "atoms, expected about %d.", actualNum, expectedNum);

    delete h;

}

void ImportanceUpdatingAgent::adjustSTIFunds(AtomSpace* a)
{
    long diff, oldTotal;
    AttentionValue::sti_t afterTax, beforeTax;
    double taxAmount;
    HandleEntry* h;
    HandleEntry* q;

    oldTotal = a->getSTIFunds();
    diff = targetLobeSTI - oldTotal;
    h = getHandlesToUpdate(a);
    taxAmount = (double) diff / (double) h->getSize();

    q = h;
    while (q) {
        int actualTax;
        actualTax = getTaxAmount(taxAmount);
        beforeTax = a->getSTI(q->handle);
        afterTax = beforeTax - actualTax;
        a->setSTI(q->handle, afterTax);
        log->fine("sti %d. Actual tax %d. after tax %d.", beforeTax, actualTax, afterTax);
        q = q->next;
    }
    delete h;

    log->info("AtomSpace STI Funds were %d, now %d. All atoms taxed %f.", \
              oldTotal, a->getSTIFunds(), taxAmount);

}

void ImportanceUpdatingAgent::adjustLTIFunds(AtomSpace* a)
{
    long diff, oldTotal;
    AttentionValue::lti_t afterTax;
    double taxAmount;
    HandleEntry* h;
    HandleEntry* q;

    oldTotal = a->getLTIFunds();
    diff = targetLobeLTI - oldTotal;
    h = getHandlesToUpdate(a);

    taxAmount = (double) diff / (double) h->getSize();

    q = h;
    while (q) {
        afterTax = a->getLTI(q->handle) - getTaxAmount(taxAmount);
        a->setLTI(q->handle, afterTax);
        q = q->next;
    }
    delete h;

    log->info("AtomSpace LTI Funds were %d, now %d. All atoms taxed %.2f.", \
              oldTotal, a->getLTIFunds(), taxAmount);
}

int ImportanceUpdatingAgent::getTaxAmount(double mean)
{
    double sum, prob, p;
    int count = 0;
    int base;
    bool negative = false;

    if (mean < 0.0) {
        negative = true;
        mean = -mean;
    }
    base = (int) mean;
    mean = mean - base;
    // Calculates tax amount by sampling a Poisson distribution
    p = getRandGen()->randDoubleOneExcluded();
    prob = sum = exp(-mean);

    while (p > sum) {
        count++;
        prob = (prob * mean) / count;
        sum += prob;
    }
    count = count + base;

    if (negative) count = -count;

    return count;
}

void ImportanceUpdatingAgent::updateSTIRent(AtomSpace* a)
{
    AttentionValue::sti_t oldSTIAtomRent;
    float focusSize = 0;
    // STIAtomRent must be adapted based on attentional focus size, or else balance btw
    // lobe STI wealth and node/link STI wealth may not be maintained

    oldSTIAtomRent = STIAtomRent;

    if (!updateLinks) {
        if (attentionalFocusNodesSize.recent > 0)
            STIAtomRent = (AttentionValue::sti_t) ceil((float) STIAtomWage * (float) totalStimulusSinceReset.recent \
                          / (float) attentionalFocusNodesSize.recent);
        //else
        //    STIAtomRent = (AttentionValue::sti_t)ceil((float) STIAtomWage * (float) totalStimulusSinceReset.recent);

        focusSize = attentionalFocusNodesSize.recent;

    } else {
        if (attentionalFocusSize.recent > 0)
            STIAtomRent = (AttentionValue::sti_t)ceil((float) STIAtomWage * (float) totalStimulusSinceReset.recent \
                          / (float) attentionalFocusSize.recent);
        //else
        //    STIAtomRent = (AttentionValue::sti_t)ceil((float) STIAtomWage * (float) totalStimulusSinceReset.recent);

        focusSize = attentionalFocusSize.recent;
    }

    log->fine("STIAtomRent was %d, now %d. Focus size was %.2f. Wage is %d. Total stim was %.2f.", oldSTIAtomRent, STIAtomRent, focusSize, STIAtomWage, totalStimulusSinceReset.recent);

    lobeSTIOutOfBounds = false;
}


void ImportanceUpdatingAgent::updateAttentionalFocusSizes(AtomSpace* a)
{
    int n = 0;
    HandleEntry* inFocus;
    HandleEntry* h;

    const AtomTable& at = a->getAtomTable();
    inFocus = at.getHandleSet(a->getAttentionalFocusBoundary() + amnesty, AttentionValue::MAXSTI);

    attentionalFocusSize.update(inFocus->getSize());

    log->fine("attentionalFocusSize = %d, recent = %f",
              attentionalFocusSize.val, attentionalFocusSize.recent);

    h = inFocus;
    while (h) {
        if (a->isNode(h->getAtom()->getType()))
            n += 1;
        h = h->next;
    }
    attentionalFocusNodesSize.update(n);

    log->fine("attentionalFocusNodesSize = %d, recent = %f",
              attentionalFocusNodesSize.val, attentionalFocusNodesSize.recent);

    delete inFocus;

}

void ImportanceUpdatingAgent::updateAtomSTI(AtomSpace* a, Handle h)
{
    AttentionValue::sti_t current, stiRentCharged, exchangeAmount;
    stim_t s;

    current = a->getSTI(h);
	stiRentCharged = calculateSTIRent(a, current);

    s = a->getAtomStimulus(h);
    exchangeAmount = - stiRentCharged + (STIAtomWage * s);
    a->setSTI(h, current + exchangeAmount);

    log->fine("Atom %s stim = %d, STI old = %d, new = %d, rent = %d", a->getName(h).c_str(), s, current, a->getSTI(h), stiRentCharged);

}

AttentionValue::sti_t ImportanceUpdatingAgent::calculateSTIRent(AtomSpace* a, AttentionValue::sti_t c)
{
    AttentionValue::sti_t stiRentCharged = 0;

	switch (rentType) {
		case RENT_FLAT:
			// Charge a flat rent to all atoms with
			// STI > AF boundary + amnesty
			if (c > a->getAttentionalFocusBoundary() + amnesty)
				stiRentCharged = STIAtomRent;
			break;
		case RENT_EXP:
			// ipython: plot(x, [max(0,i) for i in (exp(x)-(1-y))/(1+y)])
			if (c > a->getAttentionalFocusBoundary() + amnesty) {
				double y = 0.0;
				double multiplier = 0.0;
				double x;
				x = c - a->getAttentionalFocusBoundary();
				x = x / (a->getMaxSTI() - a->getAttentionalFocusBoundary());
				multiplier = max(0.0, (exp(x) - (1.0 - y))/(1.0 + y));
				stiRentCharged = (AttentionValue::sti_t) (multiplier * STIAtomRent);
			}
			break;
		case RENT_LOG:
			// max(0,i) where i = log((x-(amnesty%-0.05))*20)/2])
			// ipython: plot(x, [max(0,i) for i in log((x)*20)/2])
			if (c > a->getAttentionalFocusBoundary()) {
				double percentAmnesty = 0.05;
				double multiplier = 0.0;
				double x;
				percentAmnesty = percentAmnesty-0.05;
				x = c - a->getAttentionalFocusBoundary();
				x = x / (a->getMaxSTI() - a->getAttentionalFocusBoundary());
				if (percentAmnesty < 0) percentAmnesty = 0;
				multiplier = max(0.0, ::log((x - percentAmnesty) * 20) / 2.0);
				stiRentCharged = (AttentionValue::sti_t) (multiplier * STIAtomRent);
			}
			break;
	}
	return stiRentCharged;

}

void ImportanceUpdatingAgent::updateAtomLTI(AtomSpace* a, Handle h)
{
    /* collect LTI */
    AttentionValue::lti_t current, exchangeAmount;

    current = a->getLTI(h);
    exchangeAmount = - LTIAtomRent + (LTIAtomWage * a->getAtomStimulus(h));
    a->setLTI(h, current + exchangeAmount);

    log->fine("Atom %s LTI old = %d, new = %d", a->getName(h).c_str(), current, a->getLTI(h));
}

bool ImportanceUpdatingAgent::enforceSTICap(AtomSpace* a, Handle h)
{
    AttentionValue::sti_t current, diff;

    current = a->getSTI(h);
    if (current > STICap) {
        diff = current - STICap;
        a->setSTI(h, STICap);
        log->fine("Atom STI too high - old = %d, new = %d", current, a->getSTI(h));
        return true;
    } else if (current < -STICap) {
        diff = -STICap + current;
        a->setSTI(h, -STICap);
        log->fine("Atom STI too low - old = %d, new = %d", current, a->getSTI(h));
        return true;
    }
    return false;
}

bool ImportanceUpdatingAgent::enforceLTICap(AtomSpace* a, Handle h)
{
    AttentionValue::lti_t current, diff;

    current = a->getLTI(h);
    if (current > LTICap) {
        diff = current - LTICap;
        a->setLTI(h, LTICap);
        log->fine("Atom LTI too high - old = %d, new = %d", current, a->getSTI(h));
        return true;
    } else if (current < -LTICap) {
        diff = -LTICap + current;
        a->setLTI(h, -LTICap);
        log->fine("Atom LTI too low - old = %d, new = %d", current, a->getSTI(h));
        return true;
    }
    return false;
}

void ImportanceUpdatingAgent::setUpdateLinksFlag(bool f)
{
    updateLinks = f;
}

bool ImportanceUpdatingAgent::getUpdateLinksFlag()
{
    return updateLinks;
}

string ImportanceUpdatingAgent::toString()
{
    ostringstream s;

    s << "Importance Updating Mind Agent\n";
    s << "STIAtomRent: " << STIAtomRent << "\n";
    s << "STIAtomWage: " << STIAtomWage << "\n";
    s << "LTIAtomRent: " << LTIAtomRent << "\n";
    s << "LTIAtomWage: " << LTIAtomWage << "\n";
    s << "AV Caps (STI/LTI): " << STICap << "/" << LTICap << "\n";
    s << "Updating Links: ";
    if (updateLinks) s <<  "Yes";
    else s << "No";
    s << "\n";
    if (noiseOn)
        s << "Random stimulation on. Chance: " << noiseOdds << \
        " Amount: " << noiseUnit << "\n";
    s << "Recent Total Stim since reset: " << totalStimulusSinceReset.recent \
    << ", decay: " << totalStimulusSinceReset.decay << "\n";
    s << "Att. focus. Size: " << attentionalFocusSize.val << ", recent: " \
    << attentionalFocusSize.recent << ", recentForNodes: " \
    << attentionalFocusNodesSize.val << ", decay: " \
    << attentionalFocusSize.decay << "\n";
    s << "target (range) STI: " << targetLobeSTI << \
    "(" << acceptableLobeSTIRange[0] << "-" << acceptableLobeSTIRange[1] << \
    ") LTI: " << targetLobeLTI << \
    "(" << acceptableLobeLTIRange[0] << "-" << acceptableLobeLTIRange[1] << \
    ")\n";

    s.put(0); //null terminate the string cout
    return s.str();

}

//AttentionValue::sti_t ImportanceUpdatingAgent::getSTIAtomWage()
//{
//	return STIAtomWage;
//}

//AttentionValue::lti_t ImportanceUpdatingAgent::getLTIAtomWage()
//{
//	return LTIAtomWage;
//}

} // Namespace opencog

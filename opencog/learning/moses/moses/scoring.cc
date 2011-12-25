/*
 * opencog/learning/moses/moses/scoring.cc
 *
 * Copyright (C) 2002-2008 Novamente LLC
 * All Rights Reserved
 *
 * Written by Moshe Looks, Nil Geisweiller
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
#include "scoring.h"
#include <opencog/util/numeric.h>
#include <opencog/util/KLD.h>
#include <boost/range/algorithm/sort.hpp>
#include <boost/range/algorithm/for_each.hpp>
#include <boost/range/algorithm/transform.hpp>
#include <boost/range/adaptor/map.hpp>
#include <boost/range/adaptor/filtered.hpp>
#include <cmath>

namespace opencog { namespace moses {

using namespace std;
using boost::adaptors::map_values;
using boost::adaptors::filtered;
using boost::transform;

// helper to log a combo_tree and its behavioral score
inline void log_candidate_bscore(const combo_tree& tr,
                                 const behavioral_score& bs) {
    if(logger().getLevel() >= Logger::FINE) {
        stringstream ss_tr;
        ss_tr << "Evaluate candidate: " << tr;
        logger().fine(ss_tr.str());
        stringstream ss_bsc;
        ss_bsc << "BScored: ";
        ostream_behavioral_score(ss_bsc, bs);
        logger().fine(ss_bsc.str());
    }
}

////////////////////
// logical_bscore //
////////////////////
        
behavioral_score logical_bscore::operator()(const combo_tree& tr) const
{
    combo::complete_truth_table tt(tr, arity);
    behavioral_score bs(target.size());

    // not_equal because lower is better
    boost::transform(tt, target, bs.begin(), not_equal_to<bool>());

    return bs;
}

behavioral_score logical_bscore::best_possible_bscore() const {
    return behavioral_score(target.size(), 0);
}

///////////////////
// contin_bscore //
///////////////////
        
behavioral_score contin_bscore::operator()(const combo_tree& tr) const
{
    OTable ct(tr, cti, rng);
    behavioral_score bs(target.size());
    boost::transform(ct, target, bs.begin(),
                     [](const vertex& vl, const vertex& vr) {
                         return fabs(get_contin(vl) - get_contin(vr)); });
    return bs;
}

/////////////////////////
// occam_contin_bscore //
/////////////////////////
        
behavioral_score occam_contin_bscore::operator()(const combo_tree& tr) const
{
    OTable ct(tr, cti, rng);
    behavioral_score bs(target.size() + (occam?1:0));
    boost::transform(ct, target, bs.begin(),
                     [](const vertex& vl, const vertex& vr) {
                         return sq(get_contin(vl) - get_contin(vr)); });
    // add the Occam's razor feature
    if(occam)
        bs.back() = complexity(tr) * complexity_coef;

    // Logger
    log_candidate_bscore(tr, bs);
    // ~Logger

    return bs;
}

behavioral_score occam_contin_bscore::best_possible_bscore() const {
    return behavioral_score(target.size() + (occam?1:0), 0);
}

void occam_contin_bscore::set_complexity_coef(float stdev, float alphabet_size)
{
    if(occam)
        complexity_coef = - log(alphabet_size) * 2 * sq(stdev);
}

////////////////////////////////////
// occam_discretize_contin_bscore //
////////////////////////////////////
        
occam_discretize_contin_bscore::occam_discretize_contin_bscore
                                        (const OTable& ot,
                                         const ITable& it,
                                         const vector<contin_t>& thres,
                                         bool wa,
                                         float p,
                                         float alphabet_size,
                                         RandGen& _rng)
    : target(ot), cit(it), thresholds(thres), weighted_accuracy(wa), rng(_rng),
      classes(ot.size()), weights(thresholds.size() + 1, 1) {
    // enforce that thresholds is sorted
    boost::sort(thresholds);
    // precompute classes
    boost::transform(target, classes.begin(), [&](const vertex& v) {
            return this->class_idx(get_contin(v)); });
    // precompute weights
    multiset<size_t> cs(classes.begin(), classes.end());
    if(weighted_accuracy)
        for(size_t i = 0; i < weights.size(); ++i) {
            weights[i] = classes.size() / (float)(weights.size() * cs.count(i));
            // std::cout << "i = " << i << " weight = " << weights[i] << std::endl;
        }
    // precompute Occam's razor coefficient
    occam = p > 0 && p < 0.5;
    if(occam)
        complexity_coef = log((double)alphabet_size) / log(p/(1-p));    
}

behavioral_score occam_discretize_contin_bscore::best_possible_bscore() const {
    return behavioral_score(target.size(), 0);
}
        
size_t occam_discretize_contin_bscore::class_idx(contin_t v) const {
    if(v < thresholds[0])
        return 0;
    size_t s = thresholds.size();
    if(v >= thresholds[s - 1])
        return s;
    return class_idx_within(v, 1, s);
}

size_t occam_discretize_contin_bscore::class_idx_within(contin_t v,
                                                        size_t l_idx,
                                                        size_t u_idx) const
{
    // base case
    if(u_idx - l_idx == 1)
        return l_idx;
    // recursive case
    size_t m_idx = l_idx + (u_idx - l_idx) / 2;
    contin_t t = thresholds[m_idx - 1];
    if(v < t)
        return class_idx_within(v, l_idx, m_idx);
    else
        return class_idx_within(v, m_idx, u_idx);
}

behavioral_score occam_discretize_contin_bscore::operator()(const combo_tree& tr) const
{
    OTable ct(tr, cit, rng);
    behavioral_score bs(target.size() + (occam?1:0));
    boost::transform(ct, classes, bs.begin(), [&](const vertex& v, size_t c_idx) {
            return (c_idx != this->class_idx(get_contin(v))) * this->weights[c_idx];
        });
    // add the Occam's razor feature
    if(occam)
        bs.back() = complexity(tr) * complexity_coef;

    // Logger
    log_candidate_bscore(tr, bs);
    // ~Logger

    return bs;    
}

///////////////////////////////
// occam_ctruth_table_bscore //
///////////////////////////////
        
occam_ctruth_table_bscore::occam_ctruth_table_bscore(const CTable& _ctt,
                                                     float p,
                                                     float alphabet_size,
                                                     RandGen& _rng) 
    : ctt(_ctt), rng(_rng) {
    occam = p > 0 && p < 0.5;
    if(occam)
        complexity_coef = log((double)alphabet_size) / log(p/(1-p));
}

behavioral_score occam_ctruth_table_bscore::operator()(const combo_tree& tr) const
{
    OTable ptt(tr, ctt, rng);
    behavioral_score bs(ctt.size() + (occam?1:0));
    transform(ptt, ctt | map_values, bs.begin(),
              [](const vertex& v, CTable::mapped_type& vd) {
                  return vd[negate_vertex(v)]; });
    // add the Occam's razor feature
    if(occam)
        bs.back() = complexity(tr) * complexity_coef;

    // Logger
    log_candidate_bscore(tr, bs);
    // ~Logger
    return bs;
}

behavioral_score occam_ctruth_table_bscore::best_possible_bscore() const {
    behavioral_score bs(ctt.size() + (occam?1:0));
    transform(ctt | map_values, bs.begin(), [](CTable::mapped_type& vd) {
            return std::min(vd[id::logical_true], vd[id::logical_false]); });
    // add the Occam's razor feature
    if(occam)
        bs.back() = 0;
    return bs;
}

//////////////////////////
// occam_max_KLD_bscore //
//////////////////////////
        
occam_max_KLD_bscore::occam_max_KLD_bscore(const Table& table,
                                           float stdev,
                                           float alphabet_size,
                                           RandGen& _rng)
    : cot(table.size()), otable(table.otable), itable(table.itable), rng(_rng)
{
    occam = stdev > 0;
    set_complexity_coef(stdev, alphabet_size);
    boost::transform(table.otable, cot.begin(),
                     [](const vertex& v) { return get_contin(v); });
    boost::sort(cot);
}

behavioral_score occam_max_KLD_bscore::operator()(const combo_tree& tr) const
{
    OTable tr_ot(tr, itable, rng);

    // filter the output according to tr
    vector<contin_t> f_output;
    auto ot_it = otable.cbegin(), tr_ot_it = tr_ot.cbegin();
    for (; ot_it != otable.cend(); ++ot_it, ++tr_ot_it)
        if (vertex_to_bool(*tr_ot_it))
            f_output.push_back(get_contin(*ot_it));

    // sort the filtered output and compute KLD(cot, f_output) per
    // component
    boost::sort(f_output);
    KLDS<vector<contin_t> > klds(cot, f_output);
    behavioral_score bs;
    dorepeat(otable.size())
        bs.push_back(klds.next());
    
    // add the Occam's razor feature
    if(occam)
        bs.push_back(complexity(tr) * complexity_coef);

    // Logger
    log_candidate_bscore(tr, bs);
    // ~Logger

    return bs;
}

behavioral_score occam_max_KLD_bscore::best_possible_bscore() const
{
    return behavioral_score(1, -best_score);
}

void occam_max_KLD_bscore::set_complexity_coef(float stdev, float alphabet_size)
{
    if(occam)
        complexity_coef = - log(alphabet_size) * 2 * sq(stdev);
}

} // ~namespace moses
} // ~namespace opencog

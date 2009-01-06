/*
 * DefaultPatternMatchCB.h
 *
 * Linas Vepstas January 2009
 */

#ifndef _OPENCOG_DEFAULT_PATTERN_MATCH_H
#define _OPENCOG_DEFAULT_PATTERN_MATCH_H

#include <opencog/atomspace/types.h>
#include <opencog/atomspace/Link.h>
#include <opencog/atomspace/Node.h>
#include <opencog/query/PatternMatchCallback.h>

namespace opencog {

/**
 * Callback class, used to implement specifics of node 
 * matching, and also, to report solutions when found.
 */
class DefaultPatternMatchCB :
	public PatternMatchCallback
{
	public:
		virtual ~DefaultPatternMatchCB() {};

		/**
		 * Called when a node in the template pattern
		 * needs to be compared to a possibly matching
		 * node in the atomspace. The first argument
		 * is a node from the pattern, and the second
		 * is a possible solution node from the atomspace.
		 * Return false if the nodes match, else return
		 * true. (i.e. return true if mis-match).
		 *
		 * By default, the nodes must be identical,
		 * or one of them must be a variable.
		 */
		virtual bool node_match(Node *npat, Node *nsoln)
		{
			// If equality, then a match.
			if (npat == nsoln) return false;

			// If the ungrounded term is a variable, then OK.
			Type pattype = npat->getType();
			if (pattype == VARIABLE_NODE) return false;

			return true;
		}


		/**
		 * Called when a link in the template pattern
		 * needs to be compared to a possibly matching
		 * link in the atomspace. The first argument
		 * is a link from the pattern, and the second
		 * is a possible solution link from the atomspace.
		 * Return false if the links match, else return
		 * true. (i.e. return true if mis-match).
		 *
		 * By default, the link arity and the 
		 * link types must match.
		 */
		virtual bool link_match(Link *lpat, Link *lsoln)
		{
			if (lpat == lsoln) return false;

			if (lpat->getArity() != lsoln->getArity()) return true;
			Type pattype = lpat->getType();
			Type soltype = lsoln->getType();

			// If types differ, no match,
			if ((pattype != VARIABLE_SCOPE_LINK) && 
			    (pattype != soltype)) return true;
			return false;
		}

		/**
		 * Called when a solution is found. Should 
		 * return false to search for more solutions;
		 * or return true to terminate search.
		 */
		virtual bool solution(std::map<Handle, Handle> &pred_soln,
		                      std::map<Handle, Handle> &var_soln) = 0;
};

} // namespace opencog

#endif // _OPENCOG_DEFAULT_PATTERN_MATCH_H

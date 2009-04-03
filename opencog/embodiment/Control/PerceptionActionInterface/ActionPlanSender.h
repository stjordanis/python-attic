/*
 * opencog/embodiment/Control/OperationalPetController/ActionPlanSender.h
 *
 * Copyright (C) 2007-2008 Welter Luigi
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
#ifndef ACTIONPLANSENDER_H_
#define ACTIONPLANSENDER_H_

#include "ActionPlan.h"

namespace PerceptionActionInterface
{


class ActionPlanSender
{

public:

    virtual ~ActionPlanSender() {}

    /**
     * Sends the action plan to the target Virtual World.
     *
     * @param ationPlan a reference to the object that contains the action plan to be sent. Note that
     *     this reference may not be valid after the call of this method because caller may release the object.
     *        So, if the implementation of this method is going to store the action plan for further use, it must
     *        clone/copy the ActionPlan object.
     *
     * @return a boolean value that indicates the success (true) or failure (false) of this sending operation.
     */
    virtual bool sendActionPlan(const ActionPlan& actionPlan) = 0;

    /**
     * Sends the emotional feelings meessage to the target Virtual World.
     *
     * @param feelings The XML message already converted int a string representation
     *
     * @return a boolean value that indicates the success (true) or failure (false) of this sending operation.
     */
    virtual bool sendEmotionalFeelings(const std::string& emotionalFeelings) = 0;

};

} // namespace

#endif /*ACTIONPLANSENDER_H_*/

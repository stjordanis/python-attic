/*
 * opencog/embodiment/Control/PetInterface.h
 *
 * Copyright (C) 2007-2008 Novamente LLC
 * All Rights Reserved
 *
 * Written by Welter Luigi
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
#ifndef PET_INTERFACE_H_
#define PET_INTERFACE_H_
/**
 * PetInterface.h
 *
 * This is an abstract class to define the interface that the Pet class must
 * provide for usage by other classes (like PAI, Predavese parser and handlers, etc).
 *
 * Author: Welter Luigi
 * Copyright(c), 2007
 */

#include <opencog/atomspace/AtomSpace.h>
#include <opencog/util/Logger.h>
#include "AgentModeHandler.h"

using namespace opencog;

namespace Control
{

class PetInterface
{

public:
    virtual ~PetInterface() {}

    virtual const std::string& getPetId() const = 0;
    virtual const std::string& getExemplarAvatarId() const = 0;

    virtual AtomSpace& getAtomSpace() = 0;

    virtual void stopExecuting(const vector<string> &commandStatement, unsigned long timestamp) = 0;

    virtual bool isInLearningMode() const = 0;
    virtual void startLearning(const vector<string> &commandStatement, unsigned long timestamp) = 0;
    virtual void stopLearning(const vector<string> &commandStatement, unsigned long timestamp) = 0;

    virtual bool isExemplarInProgress() const = 0;
    virtual void startExemplar(const vector<string> &commandStatement, unsigned long timestamp) = 0;
    virtual void endExemplar(const vector<string> &commandStatement, unsigned long timestamp) = 0;

    virtual void trySchema(const vector<string> &commandStatement, unsigned long timestamp) = 0;
    virtual void reward(unsigned long timestamp) = 0;
    virtual void punish(unsigned long timestamp) = 0;

    /**
     * One handler mode shall be created to every agent mode.
     */
    virtual AgentModeHandler& getCurrentModeHandler( void ) = 0;

    virtual void setOwnerId(const string& ownerId) = 0;
    virtual void setExemplarAvatarId(const string& avatarId) = 0;
    virtual const std::string& getOwnerId() const = 0;
    virtual void setName(const string& petName) = 0;
    virtual const string& getName() const = 0;

    // functions used to set, get and verify if Pet has something in its
    // mouth, i.e., if it has grabbed something
    virtual void setGrabbedObj(const string& id) = 0;
    virtual const std::string& getGrabbedObj() = 0;
    virtual bool hasGrabbedObj() = 0;

    virtual void getAllActionsDoneInATrickAtTime(const Temporal& recentPeriod, HandleSeq& actionsDone) = 0;
    virtual void getAllObservedActionsDoneAtTime(const Temporal& recentPeriod, HandleSeq& behaviourDescriptions) = 0;
    virtual bool isNear(const Handle& objectHandle) = 0;
    virtual bool getVicinityAtTime(unsigned long timestamp, HandleSeq& petVicinity) = 0;
    virtual void getHighLTIObjects(HandleSeq& highLTIObjects) = 0;

    /**
     * This method keeps the latest object name, used by goto_obj and gonear_obj
     * when building a goto plan
     * @param target Object name amn it's position on LocalSpaceMap
     */
    virtual void setLatestGotoTarget( const std::pair<std::string, Spatial::Point>& target ) = 0;
    /**
     * Returns the latest object name used by goto_obj or gonear_obj
     * @return Object name and it's position
     */
    virtual const std::pair<std::string, Spatial::Point>& getLatestGotoTarget( void ) = 0;

    /**
     * When an avatar requests the pet to execute a trick, this
     * method will be used to register the command on RuleEngine
     * @param command The requested trick
     * @param parameters The list of arguments of the trick
     */
    virtual void setRequestedCommand(string command, vector<string> parameters) = 0;
    /**
     * Computes a speed for the pet to walk at in combo schema execution
     * (possibly based on its mood & the schema its executing, possibly with
     * random variation to make it less robotic) - this is in m/s and acc to
     * Tristan this "The range of valid values for speed is between -5m/s and
     * 30m/s" (whatever the hell traveling at "-5m/s" means)
     */
    virtual float computeWalkingSpeed() const {
        return 3.5;
    }

    /**
     * Computes an angle to be the minimal rotation for pet in combo schema execution
     * (possibly based on its mood & the schema its executing, possibly with
     * random variation to make it less robotic) - in radians
     */
    virtual float computeRotationAngle() const {
        return 0.1;
    }

    /**
     * Computes a duration for following, in seconds, e.g. based on how obedient
     * / interested / whatever the pet is
     */
    virtual float computeFollowingDuration() const {
        return 5.0;
    }

    /**
     * Return the type of the Agent (pet, humanoid, etc)
     *
     * @return agent type
     */
    virtual const std::string& getType( void ) const = 0;


    /**
     * Return the personality traits of the Agent
     *
     * @return agent traits
     */
    virtual const std::string& getTraits( void ) const = 0;


    /**
     * Save a LocalSpaceMap2D copy on the current application directory
     */
    void saveSpaceMapFile() {
        logger().log(opencog::Logger::DEBUG,  "PetInterface - saveSpaceMapFile().");
        if (!getAtomSpace().getSpaceServer().isLatestMapValid()) {
            logger().log(opencog::Logger::WARN,  "PetInterface - There is no space map yet.");
            return;
        }
        const SpaceServer::SpaceMap& sm = getAtomSpace().getSpaceServer().getLatestMap();
        static unsigned int mapCounter = 0;
        std::stringstream fileName;
        fileName << "ww_mapPersistence_";
        fileName << getPetId();
        fileName << "_";
        fileName << mapCounter;
        fileName << ".bin";
        SpaceServer::SpaceMap& map = (SpaceServer::SpaceMap&) sm;

        FILE* saveFile = fopen( fileName.str( ).c_str( ), "w+b" );
        map.save( saveFile );
        fclose( saveFile );
        ++mapCounter;
    }

};

} // Control

#endif /*PET_INTERFACE_H_*/

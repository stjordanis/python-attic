#include "ComboSelectInterpreter.h"

#include "util/Logger.h"
#include "util/exceptions.h"

using namespace Procedure;

ComboSelectInterpreter::ComboSelectInterpreter(PerceptionActionInterface::PAI& pai, opencog::RandGen& rng){
    this->comboInterpreter = new ComboInterpreter(pai, rng);
    this->next = 0;
}

ComboSelectInterpreter::ComboSelectInterpreter(VirtualWorldData::VirtualWorldState& v, opencog::RandGen& rng){
    this->comboInterpreter = new ComboInterpreter(v, rng);
    this->next = 0;
}


ComboSelectInterpreter::~ComboSelectInterpreter(){
    delete this->comboInterpreter;
}

void ComboSelectInterpreter::run(MessagingSystem::NetworkElement* ne){
    if(runningProc.empty()){ return; }

    // select the head of the map, since the RunningId is the map's key 
    idProcedureMap::iterator it = runningProc.begin();
    RunningComboSelectProcedure& rp = it->second;
    
    rp.cycle();
    logger().log(opencog::Logger::DEBUG, 
            "RunningComboSelect - Terminei o cycle.");
   
    if(!rp.isFinished()){
        
        logger().log(opencog::Logger::DEBUG, 
                "RunningComboSelect - Procedure not finished. Marking it failed.");
       
        // failed -  should be finished
        failed.insert(it->first);

    } else if(rp.getResult() != combo::id::null_vertex){
        logger().log(opencog::Logger::DEBUG, 
                "RunningComboSelect - Procedure finished.");

        if(rp.isFailed()){
            failed.insert(it->first); 
        } else {
            result.insert(make_pair(it->first, rp.getResult()));
            unifier.insert(make_pair(it->first, rp.getUnifierResult()));
        }
    } else {
        stringstream ss;
        ss << rp.getResult();
        logger().log(opencog::Logger::DEBUG, 
                "Third else - '%s'", ss.str().c_str());
    }
    runningProc.erase(it);
}

Procedure::RunningProcedureId ComboSelectInterpreter::runProcedure(
                                                const ComboProcedure& f, 
                                                const ComboProcedure& s,
                                                const std::vector<combo::vertex> arguments) {
    RunningProcedureId id(++next, COMBO_SELECT);
    runningProc.insert(make_pair(id, RunningComboSelectProcedure(*comboInterpreter, f, s, arguments)));
    return id;
}


Procedure::RunningProcedureId ComboSelectInterpreter::runProcedure(
                                                const ComboProcedure& f, 
                                                const ComboProcedure& s,
                                                const std::vector<combo::vertex> arguments, 
                                                combo::variable_unifier& vu){ 
    
    RunningProcedureId id(++next, COMBO_SELECT);
    runningProc.insert(make_pair(id, RunningComboSelectProcedure(*comboInterpreter, f, s, arguments, vu)));
    return id;
}

bool ComboSelectInterpreter::isFinished(Procedure::RunningProcedureId id){
    idProcedureMap::const_iterator it = runningProc.find(id);
    return (it == runningProc.end() || it->second.isFinished());  
}

bool ComboSelectInterpreter::isFailed(Procedure::RunningProcedureId id){
    if(failed.find(id) != failed.end()){
        return true;
    }

    idProcedureMap::const_iterator it = runningProc.find(id);
    return (it != runningProc.end() && it->second.isFinished() && it->second.isFailed());  
}

combo::vertex ComboSelectInterpreter::getResult(RunningProcedureId id){
    opencog::cassert(TRACE_INFO, isFinished(id), "ComboSelectInterpreter - Procedure '%lu' not finished.", id.getId());
    opencog::cassert(TRACE_INFO, !isFailed(id),  "ComboSelectInterpreter - Procedure '%lu' failed.", id.getId());
   
    idVertexMap::iterator it = result.find(id);

    if(it == result.end()){
        idProcedureMap::iterator runningProcIt = runningProc.find(id);
        if(runningProcIt == runningProc.end()){
            opencog::cassert(TRACE_INFO, false, "ERROR.");
        }

        opencog::cassert(TRACE_INFO, runningProcIt->second.isFinished(), "ComboSelectInterpreter - Procedure '%lu' not finished.", id.getId());
        return runningProcIt->second.getResult();
    }
    return it->second;
}

combo::variable_unifier& ComboSelectInterpreter::getUnifierResult(RunningProcedureId id){
    opencog::cassert(TRACE_INFO, isFinished(id), "ComboSelectInterpreter - Procedure '%lu' not finished.", id.getId());
    opencog::cassert(TRACE_INFO, !isFailed(id),  "ComboSelectInterpreter - Procedure '%lu' failed.", id.getId());

    idUnifierMap::iterator it = unifier.find(id);

    if(it == unifier.end()){
        idProcedureMap::iterator runningProcIt = runningProc.find(id);
        if(runningProcIt == runningProc.end()){
            //error
        } 
        return runningProcIt->second.getUnifierResult();
    }
    return it->second;
}

void ComboSelectInterpreter::stopProcedure(RunningProcedureId id){

    idProcedureMap::iterator it = runningProc.find(id);
    if(it != runningProc.end()){
//        it->second.stop();
    }
    
    std::set<RunningProcedureId>::iterator failedIt = failed.find(id);
    if(failedIt != failed.end()){
        failed.erase(failedIt);
    }

    idVertexMap::iterator resultIt = result.find(id);
    if(resultIt != result.end()){
        result.erase(resultIt);
    }

    idUnifierMap::iterator unifierIt = unifier.find(id);
    if(unifierIt != unifier.end()){
        unifier.erase(unifierIt);
    }

}

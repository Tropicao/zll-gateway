#include <stdlib.h>
#include <string.h>
#include <Eina.h>
#include "sm.h"
#include "utils.h"

ZgSm *zg_sm_create( const char *name,
                    ZgSmStateData *states,
                    ZgSmStateNb nb_states,
                    ZgSmTransitionData *transitions,
                    ZgSmTransitionNb nb_transitions)
{
    ZgSm *result = NULL;

    if(!name)
    {
        fprintf(stderr,"Cannot create state machine without proper name");
        return NULL;
    }

    if(!states || !nb_states)
    {
        fprintf(stderr,"Cannot create state machine %s with no states", name);
        return NULL;
    }

    if(!transitions || !nb_transitions)
    {
        fprintf(stderr,"Cannot create state machine %s with no transitions", name);
        return NULL;
    }

    result = calloc(1, sizeof(ZgSm));
    if(!result)
    {
        fprintf(stderr,"Cannot allocate memory for new state machine %s", name);
        return NULL;
    }

    strncpy(result->name, name, ZG_SM_NAME_MAX_LEN);
    result->states = states;
    result->nb_states = nb_states;
    result->transitions = transitions;
    result->nb_transitions = nb_transitions;
    return result;
}

uint8_t zg_sm_start(ZgSm *sm)
{
    uint8_t res = 1;
    if(sm && sm->states && sm->states[sm->current_state].func)
    {
        sm->states[sm->current_state].func();
        res = 0;
    }
    else if (sm && sm->name)
    {
        fprintf(stderr,"Cannot start state machine %s", sm->name);
    }
    else
    {
        fprintf(stderr,"Cannot start state machine because of missing data");
    }

    return res;
}

void zg_sm_destroy(ZgSm *sm)
{
    ZG_VAR_FREE(sm);
}

void zg_sm_send_event(ZgSm *sm, ZgSmEvent event)
{
    ZgSmTransitionNb index = 0;
    ZgSmState new_state;

    if(!sm)
    {
        fprintf(stderr,"No state machine associated to received event");
        return;
    }

    new_state=sm->current_state;
    for(index = 0; index < sm->nb_transitions; index++)
    {
        if(sm->transitions[index].state != sm->current_state)
            continue;

        if(sm->transitions[index].event == event)
        {
            new_state = sm->transitions[index].new_state;
            break;
        }
    }

    if(new_state != sm->current_state)
        sm->current_state = new_state;

    if(sm->states[new_state].func)
        sm->states[new_state].func();
}

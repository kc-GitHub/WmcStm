#ifndef FSMLIST_HPP_INCLUDED
#define FSMLIST_HPP_INCLUDED

#include <tinyfsm.hpp>
#include <wmc_app.h>

typedef tinyfsm::FsmList<wmcApp> fsm_list;

/* wrapper to fsm_list::dispatch() */
template <typename E> void send_event(E const& event) { fsm_list::template dispatch<E>(event); }

#endif

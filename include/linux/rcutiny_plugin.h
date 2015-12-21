#ifndef _RCUTINY_PLUGIN_H_
#define _RCUTINY_PLUGIN_H_

#include "rcupdate.h"
/*
 * Queue a preemptible - RCU callback for invocation after a grace period.
 */
void call_rcu(struct rcu_head *head,void (*func)(struct rcu_head *rcu))
{
}


#endif

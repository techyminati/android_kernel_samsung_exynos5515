#ifndef _LINUX_CPUFREQ_PMQOS_BOOST_H_
#define _LINUX_CPUFREQ_PMQOS_BOOST_H_

#ifdef CONFIG_PMQOS_BOOST
void wristup_booster_turn_on(void);
#else
static inline void wristup_booster_turn_on(void) {}
#endif 

#endif

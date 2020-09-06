#ifndef BASE_HELPER_SQL_H
#define BASE_HELPER_SQL_H

#include <base/detect.h>
#include <base/threadpool.h>

#if defined(CONF_FAMILY_UNIX)
#include <cmath>
#endif

namespace kurosio
{
    #define kpause(microsec) set_pause_function([](){}, microsec) \

    template <typename T>
    void set_timer_detach(T header, int miliseconds) 
    {
        std::thread t([=]()
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(miliseconds));
            header();
        });
        t.detach();
    }

    template <typename T>
    void set_pause_function(T header, int miliseconds) 
    {
        std::thread t([=]()
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(miliseconds));
            header();
        });
        t.join();
    }

    // derive from the number of percent e.g. ((100, 10%) = 10)
    template <typename T>
    inline float translate_to_procent_rest(T count, T procent) { return (((float)count/(float)100.0f)*(float)procent); }

    // add to the number a percentage e.g. ((100, 10%) = 110)
    template <typename T>
    inline float add_procent_to_source(T *count, float procent)
    {
 	   	*count = ((float)*count * ((float)1.0f + (procent/100.0f)));
		return (float)(*count);
    }

    // translate from the first to the second in percent e.g. ((10, 5) = 50%)
    template <typename T>
    inline float translate_to_procent(T count, T count2) { return (((float)count2 /(float)count)*(float)100.0f); }

	inline int computeExperience(int Level)
	{
        if (Level == 1)
            return 18;
        return (int)(24 * pow(Level, 2)) - (24 * Level);
	}
}

#endif
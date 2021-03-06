/* Copyright (c) 2018 Arm Limited
*
* SPDX-License-Identifier: Apache-2.0
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/
#include "mbed.h"
#include "platform/mbed_thread.h"

#if !defined(MBED_CPU_STATS_ENABLED) || !defined(DEVICE_LPTICKER) || !defined(DEVICE_SLEEP)
#error [NOT_SUPPORTED] test not supported
#endif

// Initialise the digital pin LED1 as an output
DigitalOut led1(LED1);

#define MAX_THREAD_STACK 384
#define SAMPLE_TIME_MS   2000
#define LOOP_TIME_MS     3000

uint64_t prev_idle_time = 0;
int32_t wait_time_ms = 5000;

void busy_thread()
{
    volatile uint64_t i = ~0;

    while(i--) {
        led1 = !led1;
        thread_sleep_for(wait_time_ms);
    }
}

void print_cpu_stats()
{
    mbed_stats_cpu_t stats;
    mbed_stats_cpu_get(&stats);

    // Calculate the percentage of CPU usage
    uint64_t diff_usec = (stats.idle_time - prev_idle_time);
    uint8_t idle = (diff_usec * 100) / (SAMPLE_TIME_MS*1000);
    uint8_t usage = 100 - ((diff_usec * 100) / (SAMPLE_TIME_MS*1000));
    prev_idle_time = stats.idle_time;
    
    printf("Time(us): Up: %lld", stats.uptime);
    printf("   Idle: %lld", stats.idle_time);
    printf("   Sleep: %lld", stats.sleep_time);
    printf("   DeepSleep: %lld\n", stats.deep_sleep_time);
    printf("Idle: %d%% Usage: %d%%\n\n", idle, usage);
}

int main()
{
    // Request the shared queue
    EventQueue *stats_queue = mbed_event_queue();
    Thread *thread;
    int id;

    id = stats_queue->call_every(SAMPLE_TIME_MS, print_cpu_stats);
    thread = new Thread(osPriorityNormal, MAX_THREAD_STACK);
    thread->start(busy_thread);

    // Steadily increase the system load
    for (int count = 1; ; count++) {
        thread_sleep_for(LOOP_TIME_MS);
        if (wait_time_ms <= 0) {
            break;
        }
        wait_time_ms -= 1000;
    }
    thread->terminate();
    stats_queue->cancel(id);
    return 0;
}

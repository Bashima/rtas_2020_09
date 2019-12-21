import random
import operator
import numpy as np
import math

#for the optimizer
import pandas as pd
from gurobipy import *

global_threshold = 0
count_f = 0
count_b = 0
count_succ = 0
count_fail = 0
count_total = 0 
count_edf = 0
count_lazy = 0
count_optimal = 0
count_old = 0
count_online = 0
count_online1 = 0
count_dm = 0
printable = True

class Job:
    def __init__(self, task_number, job_number, arrival_time, deadline, execution_time, energy, period):
        self.task_number = task_number
        self.job_number = job_number
        self.arrival_time = arrival_time
        self.deadline = deadline
        self.execution_time = execution_time
        self.energy = energy
        self.lazy_start_time = self.deadline - self.execution_time
        self.id = 0
        self.period = period
    
    def _print_job(self):
        print("task: ", self.task_number, "job: ", self.job_number, "arrival: ", self.arrival_time, "deadline: ", self.deadline, "exec: ", self.execution_time, "energy: ", self.energy, "id: ", self.id, " , lazy start: ", self.lazy_start_time, " period: ", self.period)

    def _set_id(self, id):
        self.id = id
class Locking_Job:
    def __init__(self, time, energy):
        self.time = time
        self.energy = energy

def _energy_status_generator(energy_status_type, duration):
    energy_status_array = []
    energy_status_array.append(1)
    for i in range(1, duration):
        energy_status_array.append(random.randint(0, energy_status_type - 1))
    return energy_status_array

def _find_devisable_list(gsc):
    dividable_array = []
    for i in range(2, gsc+1):
        if gsc%i == 0:
            dividable_array.append(i)
    return dividable_array

def _task_generator(task_count, highest_period, utilization_target):
    period_array = []
    execution_time_array = []
    deadline_array = []
    consumed_energy_rate = []
    utilization = 0.0
    probable_periods = _find_devisable_list(highest_period)
    while True:
        time_period_index = random.randint(0, len(probable_periods)-1)
        temp_period = probable_periods[time_period_index]
        temp_exec_time = random.randint(1, min(1, math.ceil(temp_period/2)))
        temp_utilization = temp_exec_time * 1.00 / temp_period
        if(utilization + temp_utilization >= utilization_target):
            if(len(probable_periods)==1):
                break
            probable_periods.remove(probable_periods[len(probable_periods)-1])
        else:
            utilization += temp_utilization
            period_array.append(temp_period) 
            execution_time_array.append(temp_exec_time)
            consumed_energy_rate.append(random.randint(3, 6))
            deadline_array.append(temp_period) # Considering deadline == period
        if(utilization <= utilization_target and utilization > 0.3):
            break
    return period_array, execution_time_array, deadline_array, consumed_energy_rate, utilization



def _calculate_energy(energy_array):
    array_len = len(energy_array)
    energy_total_array = []
    energy_total = 0
    for i in range(1, array_len+1):
        energy_total += energy_array[i-1]
        energy_total_array.append(energy_total)
    return energy_total_array

def _get_inital_lock_array(energy_array):
    lock_array = []
    for i in range(0, len(energy_array)): 
        if energy_array[i] > 0:
            lock_array.append(-1)
        else:
            lock_array.append(0)
    return lock_array

def _find_lcm(num1, num2):
    if(num1 > num2):
        greater = num1
    else:
        greater = num2
    while(True):
        if((greater%num1 == 0) and (greater % num2 ==0)):
            lcm = greater
            break
        greater += 1
    return lcm

def _lcm(period_array):
    num1 = period_array[0]
    num2 = period_array[1]

    lcm = _find_lcm(num1, num2)
    for i in range(2, len(period_array)):
        lcm = _find_lcm(lcm, period_array[i])
    return lcm 

def _job_list_generator(period_array, deadline_array, execution_time_array, energy_rate_array, hyper_period):
    global count_total
    task_number = len(period_array)
    job_array = []
    initial_id = 1
    for i in range(0, task_number):
        task_period = period_array[i]
        task_deadline = deadline_array[i]
        instance = math.ceil(hyper_period / task_period)
        for j in range(0, instance+1):
            job_arrival_time = task_period * j
            job_deadline_time = job_arrival_time + task_deadline
            if(job_deadline_time > hyper_period):
                break
            job = Job(i+1, j+1, job_arrival_time, job_deadline_time, execution_time_array[i], energy_rate_array[i], task_period)
            job._set_id(initial_id)
            initial_id += 1
            job_array.append(job)
    final_job_array = sorted(job_array, key=operator.attrgetter('arrival_time'), reverse=True)
    count_total += len(job_array)
    return job_array

def _check_schedulability(job, lock_array, total_energy_array, energy_array):
    
    temp_total_energy = total_energy_array.copy()
    exec_time = 0
    for i in range(job.arrival_time, job.deadline):
        if(lock_array[i]== 0):
            if temp_total_energy[i] >= job.energy:
                exec_time += 1
                temp_total_energy[i] -= job.energy
                for t in range(i+1, len(lock_array)):
                    temp_total_energy[t] -= job.energy
                    if temp_total_energy[t] <0:
                        return False    
    if(exec_time >= job.execution_time):
        return True
    return False

def _populate_time(job, time_array, total_energy_array):
    exec_t = job.execution_time
    for i in range(job.arrival_time, job.deadline):
        if time_array[i]==0 and total_energy_array[i]>= job.energy:
            time_array[i] = job.task_number
            for j in range(i, len(total_energy_array)):
                if(total_energy_array[j] ==0):
                    break
                total_energy_array[j] -= job.energy
            exec_t -= 1
        if(exec_t==0):
            break
    return time_array, total_energy_array

def _initialize_job(period_array, execution_time_array,deadline_array, energy_rate_array):
    hyper_period = _lcm(period_array)
    job_array = _job_list_generator(period_array, deadline_array, execution_time_array, energy_rate_array, hyper_period)
    deadline_sorted_job_array = sorted(job_array, key=operator.attrgetter('deadline'))
    return deadline_sorted_job_array

def _update_energy_array(new_energy_total_array, energy_array, t, consumed_energy):
    array_len = len(new_energy_total_array)
    lossing_energy = energy_array[t]
    for i in range(t, array_len):
        new_energy_total_array[i] -= energy_array[t]
        new_energy_total_array[i] -= consumed_energy
    return new_energy_total_array


def _backtrack(failure_job, time_array, total_energy_array, energy_array, consumed_energy_rate_array):
    global count_b
    exec_needed = failure_job.execution_time
    change_index = []
    t = failure_job.deadline-1
    new_energy_array = energy_array.copy()
    new_energy_total_array = total_energy_array.copy()
    new_time_array = time_array.copy()
    last_ind = len(time_array)-1
    
    while(t> max(0, failure_job.arrival_time-2)):
        if(time_array[t]==-1):
            
            new_energy_array[t] = 0
            temp_total_energy = new_energy_total_array.copy()
            temp_total_energy = _update_energy_array(new_energy_total_array, energy_array, t, failure_job.energy)
            for e in temp_total_energy:
                if e <0 :
                    flag = False
                    return False, change_index
            flag = True
            for i in range(t, last_ind):
                count_b += 1
                if(time_array[i]>0):
                    if temp_total_energy[i]<consumed_energy_rate_array[time_array[i]-1]:
                        flag = False
                        break
            if(flag):
                exec_needed -= 1
                change_index.append(t)
                new_total_energy_array = temp_total_energy.copy()
            else:
                new_energy_array[t] = energy_array[t]
            if(exec_needed == 0):
                break
        t -= 1
    if exec_needed == 0:
        return True, change_index
    else:
        return False, change_index


def _edf_b(period_array, execution_time_array, deadline_array, consumed_energy_rate_array, lock_array, total_energy_array, energy_array, back):
    new_energy_array = energy_array.copy()
    scheduled_task = []
    time_array = lock_array.copy()
    deadline_sorted_job_array = _initialize_job(period_array, execution_time_array, deadline_array, consumed_energy_rate_array)
    
    success_array = []
    failure_array = []
    for job in deadline_sorted_job_array:
        if(_check_schedulability(job, time_array, total_energy_array, energy_array) == True):
            success_array.append(job)
            time_array, total_energy_array = _populate_time(job, time_array, total_energy_array)
        else:
            failure_array.append(job)
    failure_array = sorted(failure_array, key=operator.attrgetter('arrival_time'))

    if(back):
        if(len(failure_array)!= 0):
            index = len(failure_array)-1
            while (index > -1):
                f_job = failure_array[index]
                response, change = _backtrack(f_job, time_array, total_energy_array, energy_array, consumed_energy_rate_array)
                if (response):
                    failure_array.remove(f_job)
                    success_array.append(f_job)
                    for ind in change:
                        time_array[ind] = f_job.task_number
                        for j in range(ind, len(total_energy_array)):
                            total_energy_array[j] -= energy_array[ind]
                            total_energy_array[j] -= job.energy
                index -= 1
    return time_array, total_energy_array, success_array, failure_array


def _generator():
    validity = 1
    while(validity<2):
        period_array, execution_time_array, deadline_array, consumed_energy_rate_array, utilization = _task_generator(20, 60, 1)
        validity = len(period_array)
        # if(validity > 1 and validity<3): #2
        # if(validity > 2 and validity<5): #4
        # if(validity > 4 and validity<9): #8
        # if(validity > 8 and validity < 17): #16
            # break
    
    if printable:
        print(period_array, execution_time_array, deadline_array, consumed_energy_rate_array, utilization)
    hyper_period = _lcm(period_array)
    energy_array = _energy_status_generator(3, hyper_period)
    # Solar with passing cars
    # energy_array_main = [0,0,1,0,0,0,1,0,1,0,0,0,1,1,1,1,2,1,2,0,1,1,2,1,1,1,3,3,2,1,1,1,1,2,3,2,3,2,1,1,2,2,2,1,2,2,1,1,1,1,2,1,1,0,0,1,0,0,0,0]
    #moving robot
    #energy_array_main = [0,0.25,0.25,0.25,0.25,0.25,1,1,2,2,2,2,2,2,2,2,2.75,2.75,2.75,2.75,2.75,2.75,3.25,3.25,3.25,3.25,3.25,3.5,3.5,3.5,3.5,3.5,3.5,3.5,3.5,3.5,3.5,3.5,3.25,3.25,3.25,3.25,3.25,2.75,2.75,2.75,2.75,2.75,2,2,2,2,2,2,2,1,1,1,0.25,0.25,0.25,0.25,0,0,0,0]

    # indexes = len(energy_array_main) - hyper_period
    # if(indexes >0):
    #     start_index = random.randint(0, indexes)
    # elif(indexes == 0):
    #     start_index = indexes
    # else:
    #     EOFError
    # energy_array = energy_array_main[start_index:]
    if printable:
        print("energy: ", energy_array)
    return period_array, execution_time_array, deadline_array, consumed_energy_rate_array, hyper_period, energy_array



def _algo_edf(period_array, execution_time_array, deadline_array, consumed_energy_rate_array, hyper_period, energy_array):
    global count_edf
    edf_energy_array = energy_array.copy()
    edf_scheduled_task = []
    hyper_period = _lcm(period_array)
    edf_time_array = np.zeros(hyper_period)
    deadline_sorted_job_array = _initialize_job(period_array, execution_time_array, deadline_array, consumed_energy_rate_array)
    available_energy = 0
    current_job_index = 0
    current_job = deadline_sorted_job_array[current_job_index]
    current_exec = current_job.execution_time
    for i in range(0, hyper_period):
        if i < current_job.deadline:
            if available_energy < current_job.energy:
                available_energy += energy_array[i]
                edf_time_array[i] = -1
            else:
                edf_time_array[i] = current_job.task_number
                current_exec -= 1
                available_energy -= current_job.energy
            if current_exec ==0:
                edf_scheduled_task.append(current_job)
                current_job_index += 1
                if(current_job_index == len(deadline_sorted_job_array)):
                    break
                current_job = deadline_sorted_job_array[current_job_index]
                current_exec = current_job.execution_time
        else:
            current_job_index += 1
            if(current_job_index == len(deadline_sorted_job_array)):
                break
            current_job = deadline_sorted_job_array[current_job_index]
            current_exec = current_job.execution_time
    count_edf += len(edf_scheduled_task)
    
def _algo_dm(period_array, execution_time_array, deadline_array, consumed_energy_rate_array, hyper_period, energy_array):
    global count_dm
    hyper_period = _lcm(period_array)
    deadline_sorted_job_array = _initialize_job(period_array, execution_time_array, deadline_array, consumed_energy_rate_array)
    
    available_energy = 0
    scheduled_job_array = []
    available_job_array = []
    uncschedulable_job_array = []
    dm_time = np.zeros(hyper_period, dtype=int)

    for t in range(0, hyper_period):
        for arrival_jobs in deadline_sorted_job_array:
            if arrival_jobs.arrival_time == t:
                available_job_array.append(arrival_jobs)
                deadline_sorted_job_array.remove(arrival_jobs)
        for deadline_jobs in available_job_array:
            if t == deadline_jobs.deadline:
                if deadline_jobs.execution_time > 0:
                    uncschedulable_job_array.append(deadline_jobs)
                    available_job_array.remove(deadline_jobs)
                else:
                    scheduled_job_array.append(deadline_jobs)
                    available_job_array.remove(deadline_jobs)
        if len(available_job_array) > 0:
            current_job = sorted(available_job_array, key=operator.attrgetter('period'))[0]
            if current_job.energy <= available_energy:
                dm_time[t] = current_job.id
                available_energy -= current_job.energy
                current_job.execution_time -= 1
                if(current_job.execution_time < 1):
                    scheduled_job_array.append(current_job)
                    available_job_array.remove(current_job)
            else:
                dm_time[t] = -1
                available_energy += energy_array[t]
    count_dm += len(scheduled_job_array)


def _algo_alap(period_array, execution_time_array, deadline_array, consumed_energy_rate_array, hyper_period, energy_array):
    global count_lazy
    lazy_energy_array = energy_array.copy()
    hyper_period = _lcm(period_array)
    lazy_time_array = np.zeros(hyper_period, dtype=int)
    deadline_sorted_job_array = _initialize_job(period_array, execution_time_array, deadline_array, consumed_energy_rate_array)
    lazy_start_sorted_job_array = sorted(deadline_sorted_job_array, key=operator.attrgetter('lazy_start_time'))
    job_exec_time = np.zeros(len(lazy_start_sorted_job_array), dtype=int)
    job_consumed_energy_rate = np.zeros(len(lazy_start_sorted_job_array), dtype=int)

    for j in range(0, len(lazy_start_sorted_job_array)):
        job = lazy_start_sorted_job_array[j]
        job._set_id(j+1)
        job_exec_time[j] = job.execution_time
        job_consumed_energy_rate[j] = job.energy
        temp_time = job.deadline - 1
        exec_time = job.execution_time
        while(True):
            if(lazy_time_array[temp_time] == 0):
                lazy_time_array[temp_time] = job.id
                exec_time -= 1
            temp_time -= 1
            if(exec_time == 0):
                break
            if(temp_time < job.arrival_time - 1):
                break
            if(temp_time < job.arrival_time - 1):
                for i in range(0, len(lazy_time_array)):
                    if lazy_time_array[i] == job.id:
                        lazy_time_array[i] = 0
                break
    
    available_energy = 0
    for i in range(0, hyper_period):
        if lazy_time_array[i] == 0:
            lazy_time_array[i] = -1
            available_energy += lazy_energy_array[i]
        else:
            if available_energy < job_consumed_energy_rate[lazy_time_array[i]-1]:
                lazy_time_array[i] = -1
                available_energy += lazy_energy_array[i]
            else:
                job_exec_time[lazy_time_array[i]-1] -= 1
                available_energy -= job_consumed_energy_rate[lazy_time_array[i]-1]
    if printable:
        print("lazy: ", lazy_time_array)
    count_non_zero = np.count_nonzero(job_exec_time)
    count_lazy += len(job_exec_time) - count_non_zero


def _find_job_by_id(job_array, id):
    for job in job_array:
        if id == job.id:
            return job
    return None

def _algo_online(period_array, execution_time_array, deadline_array, consumed_energy_rate_array, hyper_period, energy_array):
    global count_online
    global global_threshold
    online_energy_array = energy_array.copy()
    hyper_period = _lcm(period_array)
    online_time_array = np.zeros(hyper_period, dtype=int)
    deadline_sorted_job_array = _initialize_job(period_array, execution_time_array, deadline_array, consumed_energy_rate_array)
    exec_sorted_job_array = sorted(deadline_sorted_job_array, key=operator.attrgetter('execution_time'), reverse = True)
    lazy_start_sorted_job_array = sorted(exec_sorted_job_array, key=operator.attrgetter('lazy_start_time'))

    job_exec_time = np.zeros(len(lazy_start_sorted_job_array), dtype=int)
    job_consumed_energy_rate = np.zeros(len(lazy_start_sorted_job_array), dtype=int)
    job_deadline = np.zeros(len(lazy_start_sorted_job_array), dtype=int)
    threshold =  global_threshold
    tobe_considered_job_array = []
    for j in range(0, len(lazy_start_sorted_job_array)):
        job = lazy_start_sorted_job_array[j]
        job._set_id(j+1)
        job_exec_time[j] = job.execution_time
        job_deadline[j] = job.deadline
        job_consumed_energy_rate[j] = job.energy
        temp_time = job.deadline - 1
        exec_time = job.execution_time
        while(True):
            if(online_time_array[temp_time] == 0):
                online_time_array[temp_time] = job.id
                exec_time -= 1
            temp_time -= 1
            if(exec_time == 0):
                break
            if(temp_time < job.arrival_time - 1):
                for i in range(0, len(online_time_array)):
                    if online_time_array[i] == job.id:
                        online_time_array[i] = 0
                tobe_considered_job_array.append(job)
                break
    available_energy = 0

    for i in range(0, hyper_period):
        if online_time_array[i] == 0:
            if(online_energy_array[i] > threshold):
                online_time_array[i] = -1
                available_energy += online_energy_array[i]
            else:
                flag_en = True
                for more in range(i, hyper_period):
                    if online_time_array[more] > 0:
                        if available_energy >= job_consumed_energy_rate[online_time_array[more]-1]:
                            online_time_array[i] = online_time_array[more]
                            online_time_array[more] = 0
                            job_exec_time[online_time_array[i]-1] -= 1
                            available_energy -= job_consumed_energy_rate[online_time_array[i]-1]
                            flag_en = False
                        break
                if(flag_en):
                    online_time_array[i] = -1
                    available_energy += online_energy_array[i]
        else:
            if available_energy < job_consumed_energy_rate[online_time_array[i]-1]:
                changing_job = online_time_array[i]
                changing_job_object = _find_job_by_id(lazy_start_sorted_job_array, changing_job)

                if changing_job_object.lazy_start_time > i:
                    tobe_considered_job_array.append(changing_job_object)

                for f in range(i, hyper_period):
                    if online_time_array[f] == changing_job:
                        online_time_array[f] = 0

                e_flag = True
                tobe_considered_job_array = sorted(tobe_considered_job_array, key=operator.attrgetter('execution_time'))
                for considered_job in tobe_considered_job_array:
                    if considered_job.lazy_start_time < i:
                        tobe_considered_job_array.remove(considered_job)
                    else:
                        if i >= considered_job.arrival_time and i <= considered_job.deadline:
                            if available_energy >= job_consumed_energy_rate[considered_job.id-1]:
                                online_time_array[i] = considered_job.id
                                available_energy -= job_consumed_energy_rate[considered_job.id-1]
                                job_exec_time[considered_job.id-1] -= 1

                                if job_exec_time[considered_job.id-1] < 1:
                                    tobe_considered_job_array.remove(considered_job)
                                e_flag = False
                                break
                if(e_flag):
                    if online_energy_array[i] > 0:
                        online_time_array[i] = -1
                        available_energy += online_energy_array[i]



                for m in range(i+1, hyper_period):
                    if online_time_array[f] > 0:
                        for k in range(m, hyper_period):
                            if online_time_array[k] == 0:
                                if k < job_deadline[online_time_array[m]-1]:
                                    online_time_array[k] = online_time_array[m]
                                    online_time_array[m] = 0
                for j in tobe_considered_job_array:
    
                    temp_time = j.deadline - 1
                    exec_time = job_exec_time[j.id - 1]
                    while(True):
                        if(online_time_array[temp_time] == 0):
                            online_time_array[temp_time] = j.id
                            exec_time -= 1
                        temp_time -= 1
                        if(exec_time == 0):
                            tobe_considered_job_array.remove(j)
                            break
                        if(temp_time < j.arrival_time - 1) or temp_time<i:
                            for i in range(0, len(online_time_array)):
                                if online_time_array[i] == j.id:
                                    online_time_array[i] = 0
                            break
                            
            else:
                job_exec_time[online_time_array[i]-1] -= 1
                available_energy -= job_consumed_energy_rate[online_time_array[i]-1]
    if printable:
        print("online: ", online_time_array)
    count_non_zero = np.count_nonzero(job_exec_time)
    count_online += len(job_exec_time) - count_non_zero
    global_threshold = 0
    

def _algo_online_adaptive(period_array, execution_time_array, deadline_array, consumed_energy_rate_array, hyper_period, energy_array):
    global count_online1
    global global_threshold
    online_energy_array = energy_array.copy()
    hyper_period = _lcm(period_array)
    online_time_array = np.zeros(hyper_period, dtype=int)
    deadline_sorted_job_array = _initialize_job(period_array, execution_time_array, deadline_array, consumed_energy_rate_array)
    exec_sorted_job_array = sorted(deadline_sorted_job_array, key=operator.attrgetter('execution_time'), reverse = True)
    lazy_start_sorted_job_array = sorted(exec_sorted_job_array, key=operator.attrgetter('lazy_start_time'))

    job_exec_time = np.zeros(len(lazy_start_sorted_job_array), dtype=int)
    job_consumed_energy_rate = np.zeros(len(lazy_start_sorted_job_array), dtype=int)
    job_deadline = np.zeros(len(lazy_start_sorted_job_array), dtype=int)
    threshold = 0
    tobe_considered_job_array = []
    smallest_missed_energy = -10000
    smallest_missed_time = -100000
    final_unschedulable = []
    for j in range(0, len(lazy_start_sorted_job_array)):
        job = lazy_start_sorted_job_array[j]
        job._set_id(j+1)
        job_exec_time[j] = job.execution_time
        job_deadline[j] = job.deadline
        job_consumed_energy_rate[j] = job.energy
        temp_time = job.deadline - 1
        exec_time = job.execution_time
        while(True):
            if(online_time_array[temp_time] == 0):
                online_time_array[temp_time] = job.id
                exec_time -= 1
            temp_time -= 1
            if(exec_time == 0):
                break
            if(temp_time < job.arrival_time - 1):
                for i in range(0, len(online_time_array)):
                    if online_time_array[i] == job.id:
                        online_time_array[i] = 0
                tobe_considered_job_array.append(job)
                break
    available_energy = 0

    for i in range(0, hyper_period):
        if online_time_array[i] == 0:
            if(online_energy_array[i] > threshold):
                online_time_array[i] = -1
                available_energy += online_energy_array[i]
            else:
                for more in range(i, hyper_period):
                    if online_time_array[more] > 0:
                        if available_energy >= job_consumed_energy_rate[online_time_array[more]-1]:
                            online_time_array[i] = online_time_array[more]
                            online_time_array[more] = 0
                            job_exec_time[online_time_array[i]-1] -= 1
                            available_energy -= job_consumed_energy_rate[online_time_array[i]-1]
                            break
        else:
            if available_energy < job_consumed_energy_rate[online_time_array[i]-1]:
                changing_job = online_time_array[i]
                changing_job_object = _find_job_by_id(lazy_start_sorted_job_array, changing_job)

                if changing_job_object.lazy_start_time > i:                    
                    tobe_considered_job_array.append(changing_job_object)
            
                for f in range(i, hyper_period):
                    if online_time_array[f] == changing_job:
                        online_time_array[f] = 0

                e_flag = True
                tobe_considered_job_array = sorted(tobe_considered_job_array, key=operator.attrgetter('execution_time'))
                for considered_job in tobe_considered_job_array:
                    if considered_job.lazy_start_time < i:
                        smallest_missed_time = max(smallest_missed_time, considered_job.execution_time)
                        smallest_missed_energy = max(smallest_missed_energy, (considered_job.energy * considered_job.execution_time))
                        final_unschedulable.append(considered_job)
                        tobe_considered_job_array.remove(considered_job)
                    else:
                        if i >= considered_job.arrival_time and i <= considered_job.deadline:
                            if available_energy >= job_consumed_energy_rate[considered_job.id-1]:
                                online_time_array[i] = considered_job.id
                                available_energy -= job_consumed_energy_rate[considered_job.id-1]
                                job_exec_time[considered_job.id-1] -= 1

                                if job_exec_time[considered_job.id-1] < 1:
                                    smallest_missed_time = max(smallest_missed_time, considered_job.execution_time)
                                    smallest_missed_energy = max(smallest_missed_energy, (considered_job.energy * considered_job.execution_time))
                                    final_unschedulable.append(considered_job)
                                    tobe_considered_job_array.remove(considered_job)
                                e_flag = False
                                break
                                
                if(e_flag):
                    if online_energy_array[i] > 0:
                        online_time_array[i] = -1
                        available_energy += online_energy_array[i]



                for m in range(i+1, hyper_period):
                    if online_time_array[f] > 0:
                        for k in range(m, hyper_period):
                            if online_time_array[k] == 0:
                                if k < job_deadline[online_time_array[m]-1]:
                                    online_time_array[k] = online_time_array[m]
                                    online_time_array[m] = 0
                for j in tobe_considered_job_array:
    
                    temp_time = j.deadline - 1
                    exec_time = job_exec_time[j.id - 1]
                    while(True):
                        if(online_time_array[temp_time] == 0):
                            online_time_array[temp_time] = j.id
                            exec_time -= 1
                        temp_time -= 1
                        if(exec_time == 0):
                            smallest_missed_time = max(smallest_missed_time, j.execution_time)
                            smallest_missed_energy = max(smallest_missed_energy, (j.energy * j.execution_time))
                            final_unschedulable.append(j)
                            tobe_considered_job_array.remove(j)
                            break
                        if(temp_time < j.arrival_time - 1) or temp_time<i:
                            for i in range(0, len(online_time_array)):
                                if online_time_array[i] == j.id:
                                    online_time_array[i] = 0
                            break
                            
            else:
                job_exec_time[online_time_array[i]-1] -= 1
                available_energy -= job_consumed_energy_rate[online_time_array[i]-1]
    if printable:
        print("online: ", online_time_array)
    
    empty_slots = hyper_period - np.count_nonzero(online_time_array)
    if(available_energy > (max(0, smallest_missed_energy)+ min(energy_array))):
        global_threshold = min(energy_array)
    elif(empty_slots > smallest_missed_time + ((exec_sorted_job_array[0].execution_time * exec_sorted_job_array[0].energy)/max(1, min(energy_array)))):
        global_threshold = min(energy_array)

    count_non_zero = np.count_nonzero(job_exec_time)
    count_online1 += len(job_exec_time) - count_non_zero


def _algo(period_array, execution_time_array, deadline_array, consumed_energy_rate_array, hyper_period, energy_array, check):
    global count_fail
    global count_succ
    global count_old
    total_energy_array = _calculate_energy(energy_array)
    lock_array = _get_inital_lock_array(energy_array)
    time_array, new_total_energy_array, success_array, failure_array = _edf_b(period_array, execution_time_array, deadline_array, consumed_energy_rate_array, lock_array, total_energy_array, energy_array, check)
    if check:
        count_succ += len(success_array)
        count_fail += len(failure_array)
    else:
        count_old += len(success_array)
    
    if printable:
        print("offline: ",time_array)


def _opt_generate_time_list(hyper_period):
    time_list = []
    time_upper_limit = []
    for t in range(0, hyper_period):
        time_list.append("t"+ str(t+1))
        time_upper_limit.append(1)
    return time_list, time_upper_limit

def _opt_generate_job_list(energy_array, job_array, hyper_period):
    job_list = []
    regCost = [] # this will be the point for reward. Here the reward is for executing each unit
    min_times = []
    max_times = []
    energyCost = []
    for i in range(0, hyper_period):
        if energy_array[i] > 0:
            job_list.append("e"+str(i+1))
            regCost.append(0)
            min_times.append(0)
            max_times.append(1)
            energyCost.append(energy_array[i])
    for j in job_array:
        job_list.append("j"+str(j.task_number)+str(j.job_number))
        regCost.append(1/j.execution_time)
        min_times.append(0)
        max_times.append(j.execution_time)
        energyCost.append(-1 * j.energy)
    return job_list, regCost, min_times, max_times, energyCost

def _opt_generate_availability(job_list, time_list, job_array, energy_array, hyper_period):
    availability = pd.DataFrame(np.zeros((len(job_list), len(time_list)), dtype=int), index=job_list, columns=time_list)
    for i in range(0, hyper_period):
        if energy_array[i] > 0:
            availability.at['e'+str(i+1),'t'+str(i+1)] = 1
    for j in job_array:
        for i in range(j.arrival_time , j.deadline):
            availability.at["j"+str(j.task_number)+str(j.job_number),'t'+str(i+1)] = 1
    return availability

def _optimizer(period_array, execution_time_array, deadline_array, consumed_energy_rate_array, hyper_period, energy_array):
    global count_optimal
    deadline_sorted_job_array = _initialize_job(period_array, execution_time_array, deadline_array, consumed_energy_rate_array)
    time_list, time_upper_limit = _opt_generate_time_list(hyper_period)
    job_list, regCost, min_times, max_times, energyCost = _opt_generate_job_list(energy_array, deadline_sorted_job_array, hyper_period)
    availability = _opt_generate_availability(job_list, time_list, deadline_sorted_job_array, energy_array, hyper_period)

    time_requirements  = { s : time_upper_limit[i] for i,s in enumerate(time_list) }
    avail = {(w,s) : availability.loc[w,s] for w in job_list for s in time_list}
    regularCost  = { w : regCost[i] for i,w in enumerate(job_list) }

    model = Model("Job Scheduling")
    model.setParam('OutputFlag', 0)
    # ub ensures that workers are only staffed when they are available
    x = model.addVars(job_list, time_list, ub=avail, vtype=GRB.BINARY, name='x')

    time_upper_limit = model.addConstrs(((x.sum('*',s) <= time_requirements[s] for s in time_list)), name='time_requirement')
    min_timesConstr = model.addConstrs(((x.sum(w,'*') >= min_times[job_list.index(w)] for w in job_list)), name='min_times')
    max_timesConstr = model.addConstrs(((x.sum(w,'*') <= max_times[job_list.index(w)] for w in job_list)), name='max_times')
    energy_cost_cummulative = model.addVars(time_list, name='energy_cost_cumulative')

    ## for each time list of previous times
    timeArr = []
    for t in range(1, len(time_list)+1):
        tempArr = []
        for i in range(0, t):
            tempArr.append(time_list[i])
        timeArr.append(tempArr)
    # ##print(timeArr)

    for vua in range(0, hyper_period):
        s = time_list[vua]
        t_arr = timeArr[time_list.index(s)]
        energy_cost_cummulative[s] = 0
        for t in t_arr:
            for m in job_list:
                energy_cost_cummulative[s] += energyCost[job_list.index(m)] * x.sum(m,t)
                # ##print(energy_cost_cummulative[s])
        model.addConstr((energy_cost_cummulative[s] >= 0), name='energy_requirement'+str(s))

    model.ModelSense = GRB.MAXIMIZE

    Cost = 0
    Cost += (quicksum(regularCost[w]*x.sum(w,'*')  for w in job_list))

    model.setObjective(Cost)

    model.optimize()

    count_optimal += math.floor((model.ObjVal))
    

def _unit_test():
    for i in range(0, 100):
        period_array, execution_time_array, deadline_array, consumed_energy_rate_array, hyper_period, energy_array = _generator()
        _algo_dm(period_array, execution_time_array, deadline_array, consumed_energy_rate_array, hyper_period, energy_array)
        _algo_online_adaptive(period_array, execution_time_array, deadline_array, consumed_energy_rate_array, hyper_period, energy_array)
        # _algo_online(period_array, execution_time_array, deadline_array, consumed_energy_rate_array, hyper_period, energy_array)
        # _algo_online_one(period_array, execution_time_array, deadline_array, consumed_energy_rate_array, hyper_period, energy_array)
        _algo(period_array, execution_time_array, deadline_array, consumed_energy_rate_array, hyper_period, energy_array, True)
        _algo_edf(period_array, execution_time_array, deadline_array, consumed_energy_rate_array, hyper_period, energy_array)
        _algo_alap(period_array, execution_time_array, deadline_array, consumed_energy_rate_array, hyper_period, energy_array)
        _optimizer(period_array, execution_time_array, deadline_array, consumed_energy_rate_array, hyper_period, energy_array)
    # #print(_find_devisable_list(100))


def _main():
    global count_f
    global count_b
    global count_succ
    global count_fail
    global count_total
    global count_edf
    global count_optimal
    global count_old
    global count_online
    global count_online1
    global count_dm
    global printable
    global count_lazy
    
    total = 0
    optimal = 0
    offline = 0
    online = 0
    online_adaptive = 0
    edf = 0
    lazy = 0
    dm = 0
    iteration = 10
    printable = False
    print( "total, optimal, online, online_adaptive, edf, alap, dm")
    for n in range(0, iteration):
        count_f = 0
        count_b = 0
        count_succ = 0
        count_fail = 0
        count_total = 0 
        count_edf = 0
        count_lazy = 0
        count_optimal = 0
        count_old = 0
        count_online = 0
        count_online1 = 0
        count_dm = 0
        _unit_test()
        print(int(count_total/7),",", count_optimal,",", count_succ,",", count_online1,",", count_edf,",", count_dm, ",",count_lazy)
        total += int(count_total/7)
        optimal += count_optimal
        offline += count_succ
        online += count_online
        edf += count_edf
        lazy += count_lazy
        online_adaptive += count_online1
        dm += count_dm

    print(int(total/iteration),",", int(optimal/iteration),",", int(offline/iteration),",", int(online_adaptive/iteration),",", int(edf/iteration),",", int(dm/iteration),",", int(lazy/iteration))

_main()

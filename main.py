#import library 
#connect to DB
#apply DSCAN- simultanously
#on the selected community which is near to team 
#and meet the required skills look for required team
#based on team network flow

import os
import sys
import networkx as nx
import pandas
import tensorflow
import numpy
import yaml
import os
import argparse
import logging
from datetime import datetime
import time
import DSCAN
import random
import maximum_flow
import functions



def scalability_exp(data_set_lst, parameter_eps_lst, parameter_min_pts_lst, thread_num_lst, folder_name):
    data_set_lst = map(lambda name: os.pardir + os.sep + 'dataset' + os.sep + name, data_set_lst)

    print ('data set:', data_set_lst)
    for data_set in data_set_lst:
        assert 0 == os.system('ls ' + data_set + '>' + 'tmp_ls_files.txt')
    print (parameter_eps_lst)
    print (parameter_min_pts_lst)
    print (thread_num_lst)

    for data_set_path in data_set_lst:
        for eps in parameter_eps_lst:
            for min_pts in parameter_min_pts_lst:
                for thread_num in thread_num_lst:
                    data_set_name = data_set_path.split(os.sep)[-1]
                    statistics_dir = os.sep.join(
                        map(str,
                            ['.', folder_name, data_set_name, 'eps-' + str(eps), 'min_pts-' + str(min_pts)]))
                    os.system('mkdir -p ' + statistics_dir)
                    statistics_file_path = statistics_dir + os.sep + '-'.join(
                        map(str, ['output', data_set_name, eps, min_pts, thread_num])) + '.txt'

                    # 1st: splitter, record start time
                    my_splitter = '-'.join(['*' for _ in xrange(20)])
                    os.system(' '.join(
                        ['echo', my_splitter + time.ctime() + my_splitter, '>>', statistics_file_path]))

                    # 2nd: execute DSCAN with different parameters
                    params_lst = map(str, ['../DSCAN/build/DSCANParallelExp1', data_set_path,
                                           eps, min_pts, thread_num, '>>', statistics_file_path])
                    my_cmd = ' '.join(params_lst)
                    os.system(my_cmd)

                    with open(statistics_file_path, 'a+') as ifs:
                        ifs.write(my_splitter + time.ctime() + my_splitter)
                        ifs.write('\n\n\n\n')
                    print ('finish:', '-'.join(map(str, [data_set_path, eps, min_pts, thread_num])))

def find_the_cluster(data_set_lst, task_location, task_skill_lst, folder_name):
    data_set_lst = map(lambda name: os.pardir + os.sep + 'dataset' + os.sep + name, data_set_lst)

    for data_set_path in data_set_lst:
                    data_set_name = data_set_path.split(os.sep)[-1]
                    statistics_dir = os.sep.join(
                        map(str,
                            ['.', folder_name, data_set_name, 'clusters' ]))
                    os.system('mkdir -p ' + statistics_dir)
                    statistics_file_path = statistics_dir + os.sep + '-'.join(
                        map(str, ['cluster', data_set_name, task_location, task_skill_lst])) + '.txt'

                    # 1st: splitter, record start time
                    my_splitter = '-'.join(['*' for _ in xrange(20)])
                    os.system(' '.join(
                        ['echo', my_splitter + time.ctime() + my_splitter, '>>', statistics_file_path]))

                    # 2nd: find the nearest cluster which matches with required skills
                    params_lst = map(str, ['../DSCAN/build/SortCluster', data_set_path,
                                           task_location, task_skill_lst, '>>', statistics_file_path])
                    my_cmd = ' '.join(cluster_lst)
                    os.system(my_cmd)

                    with open(statistics_file_path, 'a+') as ifs:
                        ifs.write(my_splitter + time.ctime() + my_splitter)
                        ifs.write('\n\n\n\n')
                    print ('finish:', '-'.join(map(str, [task_location, task_skill_lst])))




if __name__ == '__main__':
    # parameters
    data_set_lst = [
        'Gowalla', 'Dianping',
        'Orkut-2007', 'Ljournal-2008', 'Twitter-2010'
    ]
    parameter_eps_lst = [0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9]
    parameter_min_pts_lst = [5]
    thread_num_lst = [256]
    # loop run experiments
    loop_count = 5
    for i in range(loop_count):
        scalability_exp(data_set_lst=data_set_lst, parameter_eps_lst=parameter_eps_lst,
                        parameter_min_pts_lst=parameter_min_pts_lst, thread_num_lst=thread_num_lst,
                        folder_name='DB')

    for i in range(loop_count):
        find_the_cluster(data_set_lst=data_set_lst, task_location=task_location, task_skill_lst=task_skill_lst,
                        folder_name='DB')
    
    for i in range(loop_count):
        maximum_flow(data_set_lst=data_set_lst, task_location=task_location, task_skill_lst=task_skill_lst,
                      con_Capacity=con_Capacity,Weight_skill_lst=Weight_skill_lst,
                        folder_name='DB')


#-*- coding:utf-8 -*-

import csv
import math
import numpy as np

def get_func_instr(file_name):
    
    with open(file_name,'r') as f:
        reader = csv.reader(f)
        head_1 = next(reader)

    # with open("gmssl.csv", "r") as f:
    #     reader = csv.reader(f)
    #     head_2 = next(reader)

    # print "[*] header_1 len: ", len(head_1)
    # print "[*] header_2 len: ", len(head_2)

    # head = list(set(head_1[1:]).intersection(set(head_2[1:])))

    # print "[*] header len: ", len(head)
    # head.remove("location")

    # print "\n=====================================\n"
    head = list(head_1)
    print "[*] header len: ", len(head)

    func2inst = dict()

    with open(file_name, "r") as f:
        csv_reader = csv.DictReader(f)
        for row in csv_reader:
            tmp_set = set()
            func_name = row["location"].split("/")[-1].split(".")[0]

            for k in head:
                v = row[k]
                if len(v)>0:
                    tmp_set.add(k)
                    
            if func2inst.has_key(func_name):
                func2inst[func_name] = func2inst[func_name].union(tmp_set)
            else:
                func2inst[func_name] = tmp_set
    return func2inst

"""
统计函数及其包含的指令集
:param file_name: csv文件的名字 
:return: 字典：{f1:{ins1: 12}, f2:{ins2:1}}
"""
def get_func_instr_times(file_name):
    func_instr_dict = dict()
    with open(file_name, "r") as f:
        csv_reader = csv.DictReader(f)
        for row in csv_reader:
            func_name = row["location"].split("/")[-1].split(".")[0]
            if func_instr_dict.has_key(func_name):
                for k,v in row.items():
                    if k == "location":
                        continue
                    if len(v) > 0:
                        if func_instr_dict[func_name].has_key(k):
                            func_instr_dict[func_name][k] += float(v) 
                        else:
                            func_instr_dict[func_name][k] = float(v)
            else:
                func_instr_dict[func_name] = dict()
                for k,v in row.items():
                    if k == "location":
                        continue
                    if len(v) > 0:
                        func_instr_dict[func_name][k] = float(v)

    return func_instr_dict


"""
计算两个向量之间的余弦相似度
:param vector_a: 向量 a 
:param vector_b: 向量 b
:return: sim
"""
def cos_sim(a, b):

    vector_a = []
    vector_b = []
    for i in a:
        vector_a.append(float(i))
    for i in b:
        vector_b.append(float(i))

    vector_a = np.mat(vector_a)
    vector_b = np.mat(vector_b)
    num = float(vector_a * vector_b.T)
    denom = np.linalg.norm(vector_a) * np.linalg.norm(vector_b)
    cos = num / denom
    #sim = 0.5 + 0.5 * cos
    return cos

def caculate_sim_one_hot(func_1, func_1_inst, func_2, func_2_inst):
    tmp_set = func_1_inst.union(func_2_inst)
    vector_1 = list()
    vector_2 = list()
    for item in tmp_set:
        if item in func_1_inst:
            vector_1.append(1)
        else:
            vector_1.append(0)

        if item in func_2_inst:
            vector_2.append(1)
        else:
            vector_2.append(0)
    
    # print vector_1
    # print vector_2
    # print func_1, "&" , func_2, " sim: ",  cos_sim(vector_1, vector_2)

    key_str = str(func_1+"&"+func_2+" sim: ")
    sim_value = cos_sim(vector_1, vector_2)
    return key_str, sim_value

def caculate_sim_times(func_1, func_1_inst_times, func_2, func_2_inst_times):
    tmp_list = list(set(list(func_1_inst_times.keys())).union(set(list(func_2_inst_times.keys()))))
    vector_1 = [0]*len(tmp_list)
    vector_2 = [0]*len(tmp_list)

    for item in tmp_list:
        if func_1_inst_times.has_key(item):
            vector_1[tmp_list.index(item)] = func_1_inst_times[item]

        if func_2_inst_times.has_key(item):
            vector_2[tmp_list.index(item)] = func_2_inst_times[item]

    # print vector_1
    # print vector_2
    key_str = str(func_1+"&"+func_2+" sim: ")
    sim_value = cos_sim(vector_1, vector_2)
    # if sim_value > 0.9:
    #     print key_str, " ", sim_value
    #     print vector_1
    #     print vector_2
    #     print "\n"
    return key_str, sim_value


def sim_times(f1_path, f2_path):
    ans_times_101f = get_func_instr_times(f1_path)
    print "[*] ans_times_101f's length: ", len(ans_times_101f)

    ans_times_101g = get_func_instr_times(f2_path)
    print "[*] ans_times_101g's length: ", len(ans_times_101g)

    func_sim_times = dict()
    for k1,v1 in ans_times_101f.items():
        for k2,v2 in ans_times_101g.items():
            key, value = caculate_sim_times(k1, v1, k2, v2)
            func_sim_times[key] = value

    ans = sorted(func_sim_times.items(), key=lambda item:item[1], reverse=True)
    count = 0
    for item in ans:
        # if count < 20:
        print item
        # count += 1


def sim_one_hot(f1_path, f2_path):
    ans_101f = get_func_instr(f1_path)
    print "[*] ans_times_101f's length: ", len(ans_101f)

    ans_101g = get_func_instr(f2_path)
    print "[*] ans_times_101g's length: ", len(ans_101g)

    func_sim_one_hot = dict()
    for k1,v1 in ans_101f.items():
        for k2,v2 in ans_101g.items():
            key, value = caculate_sim_one_hot(k1, v1, k2, v2)
            func_sim_one_hot[key] = value
    # print func_sim_one_hot
    ans = sorted(func_sim_one_hot.items(), key=lambda item:item[1], reverse=True)
    count = 0
    for item in ans:
        # if count < 20:
        print item
        # count += 1



if __name__ == "__main__":

    sim_times("101f.csv", "gmssl.csv")
    exit(0)

    # sim_one_hot("101f.csv", "gmssl.csv")
    # exit(0)


    # func_sim_times = dict()
    # for k1,v1 in ans_101f.items():
    #     for k2,v2 in ans_101g.items():
    #         key, value = caculate_sim_times(k1, v1, k2, v2, "101ftmp.csv", "101g.csv")
    #         func_sim_times[key] = value

    # ans = sorted(func_sim_times.items(), key=lambda item:item[1])
    # for item in ans:
    #     print item

    # intersec_dict = dict()
    # for k1,v1 in ans_101f.items():            
    #     for k2,v2 in ans_101g.items():
    #         tmp = v1.intersection(v2)
    #         if len(tmp) != 0:
    #             intersec_dict[str(k1) +"&"+ str(k2)] = tmp
    
    # ans = sorted(intersec_dict.items(), key=lambda item:item[1])

    # high = 0
    # count = len(ans) - 20
    # target_func = list()

    # for item in ans:
    #     if high < 10:
    #         print item

    #     # print "========================="

    #     if high >= count and high < len(ans):
    #         print item[0], " ", len(item[1])
    #         target_func.append(item[0].split("&")[-1])
    #     high += 1
    
    # print target_func

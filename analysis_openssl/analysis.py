#-*- coding:utf-8 -*-

import csv
import math
import numpy as np

head_1 = []
head_2 = []

with open("101f.csv",'r') as f:
    reader = csv.reader(f)
    head_1 = next(reader)

with open("101g.csv", "r") as f:
    reader = csv.reader(f)
    head_2 = next(reader)

print "[*] header_1 len: ", len(head_1)
print "[*] header_2 len: ", len(head_2)


head = list(set(head_1[1:]).intersection(set(head_2[1:])))

print "[*] header len: ", len(head)
head.remove("location")

print "\n=====================================\n"
print "[*] header len: ", len(head)

tag = 0
csv_1 = {}
with open("101ftmp.csv", "r") as f:
    #csv_reader = csv.DictReader(f,fieldnames=head)
    csv_reader = csv.DictReader(f)
    for row in csv_reader:
        key = row["location"].split("/")[-1]
        value = []

        for k in head:
            count = 0
            v = row[k]

            if len(v)>0:
                value.append(v)
            else:
                value.append(0)   
        csv_1[key] = value

# exit(1)
#  [*]hbtype
# load i16 i16*    2.0
# store i16 i16*    1.0
# alloca i16    1.0
# br i1 label label    2.0
# icmp eq i32    1.0

# load i16 i16*    2.0
# store i16 i16*    1.0
# alloca i16    1.0
# br i1 label label    2.0
# icmp eq i32    1.0

# print len(csv_1)
# exit(0)
print "====================== csv_1 ========================\n"
csv_3 = {}

with open("101g.csv", "r") as f:
    func_list = ['_ODD_bn_expand_internal', '_ODD_RSA_padding_add_PKCS1_PSS_mgf1', '_ODD_dhparam_main', '_ODD_string_to_hex', '_ODD_ssl2_generate_key_material',
            '_ODD_CMS_add1_crl', '_ODD_extract_port', '_ODD_openssl_digests', '_ODD_get_client_finished', '_ODD_CRYPTO_get_new_dynlockid',
            '_ODD_ex_class_item_LHASH_HASH', '_ODD_X509V3_string_free', '_ODD_process_pci_value', '_ODD_CAST_ecb_encrypt', '_ODD_get_client_hello',
            '_ODD_i2d_ASN1_SET', '_ODD_cfbr_encrypt_block', '_ODD_tls1_process_heartbeat', '_ODD_dtls1_heartbeat', '_ODD_dtls1_process_heartbeat']
    #csv_reader = csv.DictReader(f,fieldnames=head)
    csv_reader = csv.DictReader(f)
    for row in csv_reader:
        func_name = row["location"].split("/")[-1].split(".")[0]
        if func_name not in func_list:
            continue
            
        key = row["location"].split("/")[-1]
        value = []

        for k in head:
            count = 0
            v = row[k]

            if len(v)>0:
                value.append(v)
            else:
                value.append(0)   
        csv_3[key] = value

print "====================== csv_3 ========================\n"

# exit(0)
def cos_sim(a, b):
    """
    计算两个向量之间的余弦相似度
    :param vector_a: 向量 a 
    :param vector_b: 向量 b
    :return: sim
    """
    vector_a = []
    vector_b = []
    for i in a:
        vector_a.append(float(i))
    for i in b:
        vector_b.append(float(i))
    #print vector_a
    #print vector_b
    vector_a = np.mat(vector_a)
    vector_b = np.mat(vector_b)
    num = float(vector_a * vector_b.T)
    denom = np.linalg.norm(vector_a) * np.linalg.norm(vector_b)
    cos = num / denom
    #sim = 0.5 + 0.5 * cos
    return cos



def caculate_sim(name):
    count_1 = 0
    hit_1 = 0
    for k1,v1 in csv_1.items():
        #if name in k1 and csv_3.has_key(k1):
	for k,v in csv_3.items():
            #count_1 += 1        
            # print len(v1), len(csv_3[k1])
            # if "pdg_LCL_bp." in k1:
            #     print "csv_1_len: ", len(v1), " csv_3_len: ", len(csv_3[k1])

            #     for i in range(len(v1)):
            #         if v1[i] != 0:
            #             print i, " ", head[i], " ", v1[i]
            #             # print i, " ", v1[i]
            #     print "\n"
            #     for i in range(len(v1)):
            #         if csv_3[k1][i] != 0:
            #             print i, " ", head[i], " ", csv_3[k1][i]
            #             # print i, " ", csv_3[k1][i]
            sim = cos_sim(v1, csv_3[k])
            print k1," & ", k, " sim: ", sim
    #         if sim >= 0.6:
    #             hit_1 += 1
    # return hit_1/float(count_1)
    return sim
 
ans = caculate_sim("dtls1_process_heartbeat")   
print "dtls1_process_heartbeat ans: ", ans
print "====================================\n"


# ans = caculate_sim("convertPage")   
# print "convertPage ans: ", ans
# print "====================================\n"

# ans = caculate_sim("getAbsoluteFileName")   
# print "getAbsoluteFileName ans: ", ans
# print "====================================\n"














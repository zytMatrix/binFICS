#-*- coding:utf-8 -*-

import csv

# func_list = ['_ODD_bn_expand_internal', '_ODD_RSA_padding_add_PKCS1_PSS_mgf1', '_ODD_dhparam_main', '_ODD_string_to_hex', '_ODD_ssl2_generate_key_material',
#             '_ODD_CMS_add1_crl', '_ODD_extract_port', '_ODD_openssl_digests', '_ODD_get_client_finished', '_ODD_CRYPTO_get_new_dynlockid',
#             '_ODD_ex_class_item_LHASH_HASH', '_ODD_X509V3_string_free', '_ODD_process_pci_value', '_ODD_CAST_ecb_encrypt', '_ODD_get_client_hello',
#             '_ODD_i2d_ASN1_SET', '_ODD_cfbr_encrypt_block', '_ODD_tls1_process_heartbeat', '_ODD_dtls1_heartbeat', '_ODD_dtls1_process_heartbeat']

# lines = list()
# with open("./func_sim.txt", "r")as fp:
#     lines = fp.readlines()

# for item in func_list:
#     count = 0
#     sum_value = 0
#     for line in lines:
#         tag = "&  " + item
#         if tag in line:
#             count += 1
#             sim_value = float(line.split("sim:  ")[-1].split("\n")[0])
#             sum_value += sim_value
#             print line.split("\n")[0]
#     print "\n\naverage : ", sum_value/count, "\n"
    

#     # print " \n"
#     # exit()
    
with open("./101f.csv",'r') as f:
    reader = csv.reader(f)
    head_1 = next(reader)

for item in head_1:
    if "call" in item:
        print item

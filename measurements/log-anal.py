import re

# reg1 = "\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\stxquant\t([0-9]+)\t[0-9][0-9][0-9][0-9]-[0-9][0-9]-[0-9][0-9]T[0-9][0-9]:[0-9][0-9]:[0-9][0-9].[0-9][0-9][0-9][0-9][0-9][0-9]\sINFO\s\[blockchain\]\s\[MEASUREMENT\sblock\saccept\]\t([0-9]+)\t([0-9]+)\t([0-9]+)\t([0-9]+)\t([0-9]+)\t([0-9]+)\t([0-9]+)\t([0-9]+)"
reg1 = "\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\stxquant\t([0-9]+)\tinputsquant\t([0-9]+)\tblocksize\t([0-9]+)\t[0-9][0-9][0-9][0-9]-[0-9][0-9]-[0-9][0-9]T[0-9][0-9]:[0-9][0-9]:[0-9][0-9].[0-9][0-9][0-9][0-9][0-9][0-9]\sINFO\s\[blockchain\]\s\[MEASUREMENT\sblock\saccept\]\t([0-9]+)\t([0-9]+)\t([0-9]+)\t([0-9]+)\t([0-9]+)\t([0-9]+)\t([0-9]+)\t([0-9]+)"
reg2 = "[0-9][0-9][0-9][0-9]-[0-9][0-9]-[0-9][0-9]T[0-9][0-9]:[0-9][0-9]:[0-9][0-9].[0-9][0-9][0-9][0-9][0-9][0-9]\sINFO\s\[blockchain\]\s\[MEASUREMENT\sblock\sorganize\]\t(\w+)\t([0-9]+)\t([0-9]+)\t([0-9]+)\t([0-9]+)\t([0-9]+)\t([0-9]+)\t([0-9]+)\t([0-9]+)\t([0-9]+)\t([0-9]+)"

# fname = "/home/fernando/log.txt"
# fname = "./log2.txt"
# fname = "./log3.txt"
# fname = "./log4.txt"
# fname = "./log5.txt"
# fname = "./log6.txt"
# fname = "./log7.txt"
fname = "./log11.txt"

with open(fname) as f:
    content = f.readlines()
content = [x.strip() for x in content] 
# print(content)

start_height = 545619

print('{}\t{}\t{}\t{}\t{}\t{}\t{}\t{}\t{}\t{}\t{}\t{}\t{}\t{}\t{}\t{}\t{}\t{}\t{}\t{}\t{}\t{}'.format(
    'block hash', 'block height', 
    'tx count', 'inputs count', 'block size', 
    'check', 'handle check', 'accept', 'connect', 'handle connect', 'reorg', 'handle reorg', 'call handler', 'total',
    "t1 - t0  accept part", "t2 - t1 populate", "t3 - t2 handle_populated block->accept()", "t4 - t3 handle_populated other", "t5 - t4 handle_populated for", "t6 - t5 accept_transactions", "t7 - t6 handle_accepted", "total")
)

for i in xrange(0, len(content), 2):
    # print(content[i])
    x = re.findall(reg1, content[i])
    # print(x)
    # print(x[0])
    # print(list(x[0]))
    # break

    y = re.findall(reg2, content[i+1])
    # print(y)

    z = list(x[0]) + list(y[0])
    # print(z)

    print('{}\t{}\t{}\t{}\t{}\t{}\t{}\t{}\t{}\t{}\t{}\t{}\t{}\t{}\t{}\t{}\t{}\t{}\t{}\t{}\t{}\t{}'.format(
        z[11], start_height, 
        z[0], z[1], z[2], 
        z[13], z[14], z[15], z[16], z[17], z[18], z[19], z[20], z[21], 
        z[3], z[4], z[5], z[6], z[7], z[8], z[9], z[10])
    )

    start_height = start_height + 1




# '5637776', '22000', '20453809', '9696951', '10160', '129830862', '12673', '47087', '165711318']

# ['405', '172876', '18174336', '257837', '2560', '1348592', '432505', '1230', '20389936', '0000000000000000015daeb73e29ac79e7eca80b09a12939350c3d70ccc2c580', '94047596328460', '5637776', '22000', '20453809', '9696951', '10160', '129830862', '12673', '47087', '165711318']
# ['284', '134622', '10195801', '248107', '3583', '755149', '2282286', '765', '13620313', '000000000000000001b3dbff6f2f67c235a050c716a47d331ebb4b7f341eb114', '94047596328461', '5498306', '20793', '13706108', '23706786', '7541', '134504578', '12510', '20981', '177477603']
# ['533', '138902', '18149441', '405984', '4676', '1452858', '913292', '680', '21065833', '000000000000000000b508cacea76fb09c64e71273ef75067568dfaf39547cd4', '94047596328462', '7688700', '90348', '21124048', '10976627', '7583', '145495168', '12574', '20817', '185415865']
# ['3035', '108107', '111308762', '2379047', '6939', '259415', '10441396', '945', '124504611', '00000000000000000154fce21cb1f6c5f8a344c41bcbed1c4b6f42e400e1099a', '94047596328463', '27063061', '26249', '124590536', '42661124', '10511', '545771936', '11708', '48891', '740184016']
# ['259', '101914', '9074681', '334207', '4538', '523308', '3000936', '1320', '13040904', '000000000000000000cf315f0db0270b8adee97a643e987e3249b60cc6a3d8ed', '94047596328464', '3320210', '3374039', '13120403', '23860173', '6784', '165282399', '13741', '44405', '209022154']



# 2018-09-07T19:07:23.245188 INFO [node] Node start height is (545618).
# read_transactions_from_file - tx count:        0
# read_transactions_from_file - block count:     657

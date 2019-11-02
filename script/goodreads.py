import sys, os, glob, gzip, json, csv, random, time, psutil
from collections import OrderedDict, defaultdict

start = time.time()
idmap = OrderedDict()
def project_id(id):
	global idmap
	if not(id in idmap):
		idmap[id] = len(idmap)
	return idmap[id]

if len(sys.argv) != 4:
	print('usage: goodreads <goodreads-interactions-folder> <output-folder> <output-projection-map-file>')
	exit()
	
print('reading data...')
src_dir = sys.argv[1]
dst_dir = sys.argv[2]
dst_prj = sys.argv[3]

print('extracting transactions...')
cnt_record = 0
cnt_used_records = 0
trans = defaultdict(lambda: defaultdict(lambda: defaultdict(set)))
for infile in glob.glob(src_dir + '/goodreads_interactions_*.json.gz'):
	print('processing file ' + infile) 
	with gzip.open(infile, "r") as f:
		for line in f:
			r = json.loads(line)
			cnt_record += 1
			if cnt_record % 1048576 == 0:
				print('processed records: ' + str(cnt_record) + ', used records: ' + str(cnt_used_records) + ', memory usage: ' + str(psutil.Process(os.getpid()).memory_info().rss))
			if not r['isRead']:
				continue
			year = int(r['date_added'][-4:])-2008
			if year < 0:
				continue
			cnt_used_records += 1
			user = r['user_id']
			book = project_id(r['book_id'])
			gender = 0 if random.random()<0.5 else 1
			trans[year][gender][user].add(book)

print('dumping transactions...')
cnt_trans = 0	
for year, v in trans.items():
	for gender, v2 in v.items():
		path = dst_dir + '/year/' + str(year) + '/gender/' + str(gender)
		os.makedirs(path, exist_ok=True)
		with open(path + "/transactions.csv", "w") as f:
			w = csv.writer(f)
			for user, itemset in v2.items():
				w.writerow(itemset)
				cnt_trans += 1
				if cnt_trans % 1048576 == 0:
					print('processed transactions: ' + str(cnt_trans))

print('dumping projections...')
with open(dst_prj, 'w') as f:
	w = csv.writer(f)
	for k, v in idmap.items():
		w.writerow((v, k))

print('finished in ' + str(time.time()-start) + ' seconds')
print('total records: ' + str(cnt_record))
print('used records:  ' + str(cnt_used_records))
print('total transactions: ' + str(cnt_trans))
print('total items: ' + str(len(idmap)))

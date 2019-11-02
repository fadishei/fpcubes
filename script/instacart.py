import sys
import pandas as pd
import os
import csv

if len(sys.argv) != 3:
	print("usage: instacart <instacart-folder> <output-folser>")
	exit()

print('reading data...')
src = sys.argv[1]
dst = sys.argv[2]
orders = pd.read_csv(src+"/orders.csv", index_col=None, header=0)
order_products_prior=pd.read_csv(src+"/order_products__prior.csv", index_col=None, header=0)
order_products_train=pd.read_csv(src+"/order_products__train.csv", index_col=None, header=0)

print('preparing data...')
order_products=order_products_prior.append(order_products_train)
df=order_products.merge(orders, on="order_id")

print('extracting transactions...')
for (day, hour), sf in df.groupby(['order_dow', 'order_hour_of_day']):
	transactions = sf.groupby('order_id')['product_id'].apply(tuple).tolist()
	print((day, hour, len(transactions)))
	path = dst + "/day/" + str(day) + "/hour/" + str(hour)
	os.makedirs(path, exist_ok=True)
	with open(path + "/transactions.csv", "w") as f:
		w = csv.writer(f)
		for row in transactions:
			w.writerow(row)

import pandas as pd
import matplotlib.pyplot as plt

#---------------------------- SEQUENTIAL ----------------------- #
ll = []
for seed in range(1, 11):
    with open(f'exp-seq-e5-s{seed}') as file:
        ll += file.readlines()

for i, _ in enumerate(ll):
    ll[i] = ll[i].split()

seed = [int(z[2]) for z in ll]
usec = [int(z[5]) for z in ll]

df = pd.DataFrame({'seed' : seed, 'usec' : usec})
seq = df.mean().usec

#----------------------------------------------------------------------#

filename = 'pthread-async'

ll = []
for seed in range(1, 11):
    with open(f'exp-{filename}-e5-s{seed}') as file:
        ll += file.readlines()

for i, _ in enumerate(ll):
    ll[i] = ll[i].split()

nw = [int(z[1]) for z in ll]
seed = [int(z[3]) for z in ll]
usec = [int(z[6]) for z in ll]

df = pd.DataFrame({'nw' : nw, 'seed' : seed, 'usec' : usec})

tc = df.groupby('nw').mean().usec
thrp = 1 / tc
comp = thrp.copy()
for i in range(1, len(thrp) + 1):
    comp[i] = i / usec[1]


thrp.plot()
comp.plot()
plt.legend([filename, 'nw / $T_C^{nw = 1}$'])
plt.xlabel('Number of Workers')
plt.title(f'{filename} OES on a vector of 10^5 integers')
plt.ylabel('Throughput')
plt.show()


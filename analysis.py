import glob
import os
import sys

import matplotlib.pyplot as plt

cutoff = 1

def parse_data_name(path):
    global cutoff
    fname = os.path.split(path)[1]
    alg, opt, cut, _ = fname.split('.')
    cutoff = float(cut)

    return alg, opt

def collect_data():
    if not os.path.exists("data") or not os.path.isdir("data"):
        return {}, {}
    
    data = {}
    optset = set()
    for file in glob.glob("data/*.dat"):
        alg, opt = parse_data_name(file)
        optset.add(opt)
        data[alg, opt] = ([], [])
        with open(file, 'r') as dat:
            for line in dat.readlines():
                n, _, time = map(lambda s: s.strip(), line.split('::'))
                data[alg, opt][0].append(int(n))
                data[alg, opt][1].append(float(time))
    
    return data, optset

def restricted_max(xys):
    global cutoff
    xs, ys = xys
    imax = max(filter(lambda i: ys[i] <= cutoff, range(len(xs))), key=lambda i: ys[i])
    return xs[imax]

if __name__ == "__main__":
    data, optset = collect_data()
    if len(data) == 0:
        print("No data found!", file=sys.stderr)
        exit(-1)

    fig, axs = plt.subplots(1, len(optset))
    axmap = {}
    for idx, opt in enumerate(optset):
        axmap[opt] = axs[idx] if len(optset) > 1 else axs
        axmap[opt].set_title(opt)

    fig.suptitle("Runtimes")
    
    #plt.xscale('log')
    for alg, opt in sorted(data):
        axmap[opt].plot(data[alg, opt][0], data[alg, opt][1], label=f"{alg}")
    for alg, opt in sorted(data, key=lambda p: restricted_max(data[p])):        
        print(f"{alg}[{opt}]:", restricted_max(data[alg, opt]))

    for opt in axmap:
        axmap[opt].legend()
    plt.show()

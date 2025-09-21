import matplotlib.pyplot as plt

def plot_flows(filename="throughput.txt", flow_ids=(0, 1), skip=1, output="throughput.png"):
    data = {fid: ([], []) for fid in flow_ids}  # {flow_id: (times, throughputs)}

    with open(filename, "r") as f:
        for line in f:
            parts = line.strip().split()
            if len(parts) != 3:
                continue
            t, fid, snd = parts
            fid = int(fid)
            if fid in data:
                data[fid][0].append(int(t) - 2e9)
                data[fid][1].append(float(snd)*8 / 10e4) # turn to throughput 

    plt.figure(figsize=(10, 5))

    for fid in flow_ids:
        times = data[fid][0][::skip]
        throughputs = data[fid][1][::skip]
        plt.plot(times, throughputs, marker='o', linestyle='-', linewidth=1, markersize=2, label=f"Flow {fid}")

    plt.xlabel("Time (ns)")
    plt.ylabel("Throughput (Gbps)")
    plt.title("Throughput over Time (Flows: 2 Hosts → 1 Host, Across DCs) \n DCQCN")
    plt.legend()
    plt.grid(True)
    plt.tight_layout()
    plt.savefig(output, dpi=300)
    plt.close()

if __name__ == "__main__":
    plot_flows("throughput.txt", flow_ids=(0, 1), skip=1, output="throughput.png")
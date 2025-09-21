import matplotlib.pyplot as plt
import numpy as np

def process_file(filename, bins=50, range_min=None, range_max=None):
    values = []
    with open(filename, "r") as f:
        for line in f:
            parts = line.strip().split()
            if len(parts) < 2:
                continue
            # 倒数第二个数
            values.append(int(parts[-2]))

    values = np.array(values)

    print(f"数据总数: {len(values)}")
    print(f"最小值: {values.min()}, 最大值: {values.max()}, 平均值: {values.mean():.2f}")

    # 计算 99% 分位数
    p99 = np.percentile(values, 99)
    print(f"99% 分位数 (P99): {p99:.2f}")

    # 设置范围
    range_tuple = None
    if range_min is not None and range_max is not None:
        range_tuple = (range_min, range_max)

    # 画直方图 (百分比)
    counts, bins, patches = plt.hist(
        values,
        bins=bins,
        range=range_tuple,
        weights=np.ones(len(values)) / len(values),
        edgecolor="black"
    )

    plt.gca().yaxis.set_major_formatter(plt.FuncFormatter(lambda y, _: '{:.0%}'.format(y)))
    plt.xlabel("Value")
    plt.ylabel("Percentage")
    plt.title("Histogram of Second Last Numbers (Percentage)")
    plt.savefig("hist.png")

if __name__ == "__main__":
    process_file("fct_topology_flow_dcqcn.txt", bins=5000, range_min=0, range_max=5000000)
#include <iostream>
#include <fstream>
using namespace std;

int main() {
    int n; // server数量
    cout << "请输入server数量: ";
    cin >> n;

    ofstream fout("flow.txt");
    if (!fout) {
        cerr << "无法创建文件 flow.txt" << endl;
        return 1;
    }

    // 总flow数量（每个host到最后一个host，除去最后一个本身）
    fout << n - 1 << endl;

    int pg = 3;                     // 流量等级，可自定义
    int dport = 5000;               // 默认目的端口
    int maxPacketCount = 100000;    // 每个flow的最大包数
    double start_time = 0.1;        // 起始时间，可以设置递增

    int dst = n - 1; // 最后一个host作为接收端
    for (int i = 0; i < n; i++) {
        if (i == dst) continue; // 最后一个host不生成flow
        fout << i << " " << dst << " "
             << pg << " " << dport << " "
             << maxPacketCount << " " << start_time
             << endl;
    }

    fout.close();
    cout << "已生成 flow.txt 文件" << endl;
    return 0;
}
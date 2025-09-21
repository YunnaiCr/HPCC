#include <iostream>
#include <fstream>
using namespace std;

int main() {
    int n; // 节点数量
    cout << "请输入节点数量: ";
    cin >> n;

    ofstream fout("trace.txt");
    if (!fout) {
        cerr << "无法创建文件 trace.txt" << endl;
        return 1;
    }

    // 总trace数量
    fout << n << endl;

    for (int i = 0; i < n; i++) {
        fout << i << " ";
    }

    fout.close();
    cout << "已生成 trace.txt 文件" << endl;
    return 0;
}
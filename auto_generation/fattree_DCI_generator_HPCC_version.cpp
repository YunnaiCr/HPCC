#include <iostream>
#include <fstream>
#include <vector>
#include <stdexcept>
#include <string>

// Fat-Tree + DCI topology generator (with fallback to plain Fat-tree when num_dcs == 1)
// Modified output format:
// Line 1: <total_nodes> <total_switches> <total_links>
// Line 2: <switch ids separated by space> (including DCI if present)
// Line 3 onwards: <u> <v> <bandwidth> <delay> <loss>
// Notes:
//   - All non-DCI links: 100Gbps 1us 0.000000
//   - DCI-DCI links: (100Gbps * num_core_per_dc) 400us 0.000000
//   - Core-to-DCI links: 100Gbps 1us 0.000000 (only if num_dcs > 1)
//   - One DCI switch per DC if num_dcs > 1
//   - Each core switch in a DC connects to that DC's DCI switch
//   - DCI switches are fully meshed among themselves

class FatTreeDCIGenerator {
private:
    int k;
    int num_dcs;

    int half_k;
    int num_pods;

    // Per-DC quantities
    int edges_per_pod;
    int aggs_per_pod;
    int cores_per_dc;

    int servers_per_edge;
    int servers_per_pod;
    int hosts_per_dc;

    int edges_per_dc;
    int aggs_per_dc;
    int switches_per_dc;

    // Global totals
    long long total_hosts = 0;
    long long total_edges = 0;
    long long total_aggs = 0;
    long long total_cores = 0;
    long long total_dci = 0;
    long long total_switches = 0;
    long long total_nodes = 0;
    long long total_links = 0;

    // ID ranges
    long long host_start = 0;
    long long edge_start = 0;
    long long agg_start  = 0;
    long long core_start = 0;
    long long dci_start  = 0;

    std::vector<std::string> connections;

public:
    FatTreeDCIGenerator(int k_val, int dc_count) : k(k_val), num_dcs(dc_count) {
        if (k % 2 != 0) {
            throw std::invalid_argument("Error: k must be an even number.");
        }
        if (num_dcs <= 0) {
            throw std::invalid_argument("Error: number of DCs must be positive.");
        }

        half_k = k / 2;
        num_pods = k;

        edges_per_pod = half_k;
        aggs_per_pod  = half_k;
        cores_per_dc  = half_k * half_k;

        servers_per_edge = half_k;
        servers_per_pod  = edges_per_pod * servers_per_edge;
        hosts_per_dc     = num_pods * servers_per_pod;

        edges_per_dc     = num_pods * edges_per_pod;
        aggs_per_dc      = num_pods * aggs_per_pod;
        switches_per_dc  = edges_per_dc + aggs_per_dc + cores_per_dc;

        total_hosts    = 1LL * hosts_per_dc * num_dcs;
        total_edges    = 1LL * edges_per_dc * num_dcs;
        total_aggs     = 1LL * aggs_per_dc * num_dcs;
        total_cores    = 1LL * cores_per_dc * num_dcs;
        total_dci      = (num_dcs == 1 ? 0 : num_dcs);
        total_switches = total_edges + total_aggs + total_cores + total_dci;
        total_nodes    = total_hosts + total_switches;

        host_start = 0;
        edge_start = host_start + total_hosts;
        agg_start  = edge_start + total_edges;
        core_start = agg_start + total_aggs;
        dci_start  = core_start + total_cores;
    }

    void generate() {
        connections.clear();

        for (int dc = 0; dc < num_dcs; ++dc) {
            const long long dc_host_start = host_start + 1LL * dc * hosts_per_dc;
            const long long dc_edge_start = edge_start + 1LL * dc * edges_per_dc;
            const long long dc_agg_start  = agg_start  + 1LL * dc * aggs_per_dc;
            const long long dc_core_start = core_start + 1LL * dc * cores_per_dc;

            generateServerToEdge(dc_host_start, dc_edge_start);
            generateEdgeToAgg(dc_edge_start, dc_agg_start);
            generateAggToCore(dc_agg_start, dc_core_start);

            if (num_dcs > 1) {
                long long dci_id = dci_start + dc;
                generateCoreToDCI(dc_core_start, dci_id);
            }
        }

        if (num_dcs > 1) {
            generateDCIMesh();
        }

        total_links = (long long)connections.size();
        writeToFile();
    }

    void printSummary() const {
        std::cout << "\n=== Fat-Tree + DCI Topology Summary ===\n";
        std::cout << "k: " << k << ", DCs: " << num_dcs << "\n";
        std::cout << "Totals: switches=" << total_switches
                  << " (edges=" << total_edges
                  << ", aggs=" << total_aggs
                  << ", cores=" << total_cores
                  << ", dci=" << total_dci << ")\n";
        std::cout << "Total nodes: " << total_nodes << "\n";
        std::cout << "Total links: " << total_links << "\n";
        std::cout << "ID ranges:\n";
        std::cout << "  switches: " << edge_start << " - " << dci_start + total_dci - 1 << "\n";
    }

private:
    static std::string linkLine(long long u, long long v,
                                const std::string &bw,
                                const std::string &delay,
                                const std::string &loss = "0.000000") {
        return std::to_string(u) + " " + std::to_string(v) + " " + bw + " " + delay + " " + loss;
    }

    void generateServerToEdge(long long dc_host_start, long long dc_edge_start) {
        const std::string BW = "100Gbps";
        const std::string DELAY = "1us";
        long long server_id = dc_host_start;
        for (int p = 0; p < num_pods; ++p) {
            long long pod_edge_start = dc_edge_start + 1LL * p * edges_per_pod;
            for (int e = 0; e < edges_per_pod; ++e) {
                long long edge_id = pod_edge_start + e;
                for (int s = 0; s < servers_per_edge; ++s) {
                    connections.push_back(linkLine(server_id++, edge_id, BW, DELAY));
                }
            }
        }
    }

    void generateEdgeToAgg(long long dc_edge_start, long long dc_agg_start) {
        const std::string BW = "100Gbps";
        const std::string DELAY = "1us";
        for (int p = 0; p < num_pods; ++p) {
            long long pod_edge_start = dc_edge_start + 1LL * p * edges_per_pod;
            long long pod_agg_start  = dc_agg_start  + 1LL * p * aggs_per_pod;
            for (int e = 0; e < edges_per_pod; ++e) {
                long long edge_id = pod_edge_start + e;
                for (int a = 0; a < aggs_per_pod; ++a) {
                    long long agg_id = pod_agg_start + a;
                    connections.push_back(linkLine(edge_id, agg_id, BW, DELAY));
                }
            }
        }
    }

    void generateAggToCore(long long dc_agg_start, long long dc_core_start) {
        const std::string BW = "100Gbps";
        const std::string DELAY = "1us";
        for (int core_index = 0; core_index < cores_per_dc; ++core_index) {
            int core_j = core_index % half_k;
            long long core_id = dc_core_start + core_index;
            for (int p = 0; p < num_pods; ++p) {
                long long agg_id = dc_agg_start + 1LL * p * aggs_per_pod + core_j;
                connections.push_back(linkLine(agg_id, core_id, BW, DELAY));
            }
        }
    }

    void generateCoreToDCI(long long dc_core_start, long long dci_id) {
        const std::string BW = "100Gbps";
        const std::string DELAY = "1us";
        for (int c = 0; c < cores_per_dc; ++c) {
            long long core_id = dc_core_start + c;
            connections.push_back(linkLine(core_id, dci_id, BW, DELAY));
        }
    }

    void generateDCIMesh() {
        long long bw_val = 100LL * cores_per_dc;
        std::string BW = std::to_string(bw_val) + "Gbps";
        const std::string DELAY = "400us";
        for (int i = 0; i < num_dcs; ++i) {
            long long u = dci_start + i;
            for (int j = i + 1; j < num_dcs; ++j) {
                long long v = dci_start + j;
                connections.push_back(linkLine(u, v, BW, DELAY));
            }
        }
    }

    void writeToFile(const std::string &filename = "topology.txt") {
        std::ofstream out(filename);
        if (!out.is_open()) {
            throw std::runtime_error("Error: Could not open output file.");
        }

        out << total_nodes << " " << total_switches << " " << total_links << "\n";

        for (long long i = 0; i < total_switches; ++i) {
            if (i) out << ' ';
            out << (edge_start + i);
        }
        out << "\n";

        for (const auto &line : connections) {
            out << line << "\n";
        }

        out.close();
        std::cout << "Topology successfully written to '" << filename << "'\n";
    }
};

int main() {
    try {
        int k, dc_count;
        std::cout << "Enter Fat-Tree k (even): ";
        std::cin >> k;
        std::cout << "Enter number of DCs: ";
        std::cin >> dc_count;

        FatTreeDCIGenerator gen(k, dc_count);
        gen.generate();
        gen.printSummary();

    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    return 0;
}

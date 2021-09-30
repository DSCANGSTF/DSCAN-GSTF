#include "Graph.h"

#if defined(__INTEL_COMPILER)
#include <malloc.h>
#else

#include <mm_malloc.h>

#endif // defined(__GNUC__)

#include <cassert>
#include <cmath>
#include <cstring>

#include <algorithm>
#include <map>

#include "playground/pretty_print.h"

#include "ThreadPool.h"
#include "util/log/log.h"
#include "util/util.h"

using namespace std::chrono;
using namespace yche;

Graph::Graph(const char *dir_string, const char *eps_s, int min_u) {
    io_helper_ptr = yche::make_unique<InputOutput>(dir_string);
    io_helper_ptr->ReadGraph();

    auto tmp_start = high_resolution_clock::now();
    // 1st: parameter
    std::tie(eps_a2, eps_b2) = io_helper_ptr->ParseEps(eps_s);
    this->min_u = min_u;

    // 2nd: graph
    // csr representation
    n = static_cast<ui>(io_helper_ptr->n);
    out_edge_start = std::move(io_helper_ptr->offset_out_edges);
    out_edges = std::move(io_helper_ptr->out_edges);

    // vertex properties
    degree = std::move(io_helper_ptr->degree);
    core_status_lst = vector<char>(n, UN_KNOWN);

    // edge properties
    min_cn = static_cast<int *>(_mm_malloc(io_helper_ptr->m * sizeof(int), 32));
#define PTR_TO_UINT64(x) (uint64_t)(uintptr_t)(x)
    assert(PTR_TO_UINT64(min_cn) % 32 == 0);

    // 3rd: disjoint-set, make-set at the beginning
    disjoint_set_ptr = yche::make_unique<DisjointSets>(n);

    // 4th: cluster_dict
    cluster_dict = static_cast<int *>(_mm_malloc(io_helper_ptr->n * sizeof(int), 32));
    for (auto i = 0; i < io_helper_ptr->n; i++) {
        cluster_dict[i] = n;
    }
    assert(PTR_TO_UINT64(cluster_dict) % 32 == 0);

    auto all_end = high_resolution_clock::now();
    cout << "other construct time:" << duration_cast<milliseconds>(all_end - tmp_start).count()
         << " ms\n";
}

Graph::~Graph() {
    _mm_free(min_cn);
    _mm_free(cluster_dict);
}

void Graph::Output(const char *eps_s, const char *miu) {
    io_helper_ptr->Output(eps_s, miu, noncore_cluster, core_status_lst, cluster_dict, *disjoint_set_ptr);
}

int Graph::ComputeCnLowerBound(int du, int dv) {
    auto c = (int) (sqrtl((((long double) du) * ((long double) dv) * eps_a2) / eps_b2));
    if (((long long) c) * ((long long) c) * eps_b2 < ((long long) du) * ((long long) dv) * eps_a2) { ++c; }
    return c;
}

bool Graph::IsDefiniteCoreVertex(int u) {
    return core_status_lst[u] == CORE;
}

ui Graph::BinarySearch(EdgeVec &array, ui offset_beg, ui offset_end, int val) {
    auto mid = static_cast<ui>((static_cast<unsigned long>(offset_beg) + offset_end) / 2);
    if (array[mid] == val) { return mid; }
    return val < array[mid] ? BinarySearch(array, offset_beg, mid, val) : BinarySearch(array, mid + 1, offset_end, val);
}

void Graph::PrintMinCnBeauty() {
    map<pair<int, int>, int> dict;
    map<pair<int, int>, int> dict2;
    for (auto u = 0; u < n; u++) {
        for (auto i = out_edge_start[u]; i < out_edge_start[u + 1]; i++) {
            dict.emplace(make_pair(u, out_edges[i]), min_cn[i]);
            if (u < out_edges[i]) {
                dict2.emplace(make_pair(u, out_edges[i]), min_cn[i]);
            }
        }
    }
    stringstream ss;
    ss << dict;
    log_info("min-cn: %s", ss.str().c_str());
    reset(ss);
    ss << dict2;
    log_info("min-cn: %s", ss.str().c_str());
}

void Graph::PruneDetail(int u) {
    auto sd = 0;
    auto ed = degree[u] - 1;
    for (auto edge_idx = out_edge_start[u]; edge_idx < out_edge_start[u + 1]; edge_idx++) {
        auto v = out_edges[edge_idx];
        int deg_a = degree[u], deg_b = degree[v];
        if (deg_a > deg_b) { swap(deg_a, deg_b); }
        if (((long long) deg_a) * eps_b2 < ((long long) deg_b) * eps_a2) {
            min_cn[edge_idx] = NOT_SIMILAR;
            ed--;
        } else {
            int c = ComputeCnLowerBound(deg_a, deg_b);
            auto is_similar_flag = c <= 2;
            min_cn[edge_idx] = is_similar_flag ? SIMILAR : c;
            if (is_similar_flag) {
                sd++;
            }
        }
    }
    log_info("u: %d, sd:%d, ed:%d, sd>=min_u: %d, ed<min_u:%d", u, sd, ed, sd >= min_u, ed < min_u);

    if (sd >= min_u) {
        core_status_lst[u] = CORE;
    } else if (ed < min_u) {
        core_status_lst[u] = NON_CORE;
    }
}

void Graph::CheckCoreFirstBSP(int u) {
    if (core_status_lst[u] == UN_KNOWN) {
        auto sd = 0;
        auto ed = degree[u] - 1;
        for (auto edge_idx = out_edge_start[u]; edge_idx < out_edge_start[u + 1]; edge_idx++) {
            // be careful, the next line can only be commented when memory load/store of min_cn is atomic, no torn read
//            auto v = out_edges[edge_idx];
//            if (u <= v) {
            if (min_cn[edge_idx] == SIMILAR) {
                ++sd;
                if (sd >= min_u) {
                    log_info("min-max-pruning-sd-ed (CORE), %d, %d, %d", u, sd, ed);
                    core_status_lst[u] = CORE;
                    return;
                }
            } else if (min_cn[edge_idx] == NOT_SIMILAR) {
                --ed;
                if (ed < min_u) {
                    log_info("min-max-pruning-sd-ed (NON-CORE), %d, %d, %d", u, ed, ed);
                    core_status_lst[u] = NON_CORE;
                    return;
                }
            }
//            }
        }
        log_info("init-sd-ed, %d, %d, %d", u, sd, ed);
        for (auto edge_idx = out_edge_start[u]; edge_idx < out_edge_start[u + 1]; edge_idx++) {
            auto v = out_edges[edge_idx];
            if (u <= v && min_cn[edge_idx] > 0) {
                min_cn[edge_idx] = EvalSimilarity(u, edge_idx);
                min_cn[BinarySearch(out_edges, out_edge_start[v], out_edge_start[v + 1], u)] = min_cn[edge_idx];
                if (min_cn[edge_idx] == SIMILAR) {
                    ++sd;
                    if (sd >= min_u) {
                        log_info("early-finalize (CORE), u:%d, sd:%d, ed:%d", u, sd, ed);
                        core_status_lst[u] = CORE;
                        return;
                    }
                } else {
                    --ed;
                    if (ed < min_u) {
                        log_info("early-finalize (NON-CORE), u:%d, sd:%d, ed:%d", u, sd, ed);
                        core_status_lst[u] = NON_CORE;
                        return;
                    }
                }
            }
        }
        log_info("finalize (NON-SURE), u:%d, sd: %d, ed:%d", u, sd, ed);
    }
}

void Graph::CheckCoreSecondBSP(int u) {
    if (core_status_lst[u] == UN_KNOWN) {
        auto sd = 0;
        auto ed = degree[u] - 1;
        for (auto edge_idx = out_edge_start[u]; edge_idx < out_edge_start[u + 1]; edge_idx++) {
            if (min_cn[edge_idx] == SIMILAR) {
                ++sd;
                if (sd >= min_u) {
                    log_info("min-max-pruning-sd-ed (CORE), %d, %d, %d", u, sd, ed);
                    core_status_lst[u] = CORE;
                    return;
                }
            }
            if (min_cn[edge_idx] == NOT_SIMILAR) {
                --ed;
                if (ed < min_u) {
                    log_info("min-max-pruning-sd-ed (NON-CORE), %d, %d, %d", u, sd, ed);
                    return;
                }
            }
        }

        for (auto edge_idx = out_edge_start[u]; edge_idx < out_edge_start[u + 1]; edge_idx++) {
            auto v = out_edges[edge_idx];
            if (min_cn[edge_idx] > 0) {
                min_cn[edge_idx] = EvalSimilarity(u, edge_idx);
                min_cn[BinarySearch(out_edges, out_edge_start[v], out_edge_start[v + 1], u)] = min_cn[edge_idx];
                if (min_cn[edge_idx] == SIMILAR) {
                    ++sd;
                    if (sd >= min_u) {
                        log_info("finalize (CORE), u:%d, sd: %d, ed:%d", u, sd, ed);
                        core_status_lst[u] = CORE;
                        return;
                    }
                } else {
                    --ed;
                    if (ed < min_u) {
                        log_info("finalize (NON-CORE), u:%d, sd: %d, ed:%d", u, sd, ed);
                        return;
                    }
                }
            }
        }
    }
}

void Graph::ClusterCoreFirstPhase(int u) {
    for (auto j = out_edge_start[u]; j < out_edge_start[u + 1]; j++) {
        auto v = out_edges[j];
        bool core_v = IsDefiniteCoreVertex(v);
        bool same_set = disjoint_set_ptr->IsSameSet(static_cast<uint32_t>(u),
                                                    static_cast<uint32_t>(v));
        if (u < v && core_v)
                log_info("u:%d, v:%d, SameSet: %d, Pruning: %d", u, v, same_set, core_v && same_set ? 1 : 0);
        if (u < v && IsDefiniteCoreVertex(v) && !disjoint_set_ptr->IsSameSet(static_cast<uint32_t>(u),
                                                                             static_cast<uint32_t>(v))) {
            if (min_cn[j] == SIMILAR) {
                disjoint_set_ptr->Union(static_cast<uint32_t>(u), static_cast<uint32_t>(v));
                stringstream ss;
                ss << *disjoint_set_ptr;
                log_info("union u: %d, v:%d, \n%s", u, v, ss.str().c_str());
            }
        }
    }
}

void Graph::ClusterCoreSecondPhase(int u) {
    for (auto edge_idx = out_edge_start[u]; edge_idx < out_edge_start[u + 1]; edge_idx++) {
        auto v = out_edges[edge_idx];
        bool core_v = IsDefiniteCoreVertex(v);
        bool same_set = disjoint_set_ptr->IsSameSet(static_cast<uint32_t>(u),
                                                    static_cast<uint32_t>(v));
        if (u < v && core_v)
                log_info("u:%d, v:%d, SameSet: %d, Pruning: %d", u, v, same_set, core_v && same_set ? 1 : 0);
        if (u < v && IsDefiniteCoreVertex(v) && !disjoint_set_ptr->IsSameSet(static_cast<uint32_t>(u),
                                                                             static_cast<uint32_t>(v))) {
            if (min_cn[edge_idx] > 0) {
                min_cn[edge_idx] = EvalSimilarity(u, edge_idx);
                log_info("eval u: %d, v:%d", u, v);
                if (min_cn[edge_idx] == SIMILAR) {
                    disjoint_set_ptr->Union(static_cast<uint32_t>(u), static_cast<uint32_t>(v));
                    stringstream ss;
                    ss << *disjoint_set_ptr;
                    log_info("union u: %d, v:%d, \n%s", u, v, ss.str().c_str());
                }
            }
        }
    }
}

void Graph::ClusterNonCoreDetail(int u, vector<pair<int, int>> &tmp_cluster) {
    for (auto j = out_edge_start[u]; j < out_edge_start[u + 1]; j++) {
        auto v = out_edges[j];
        if (u == 2) {
            log_info("%d, %d", v, !IsDefiniteCoreVertex(v));
        }
        if (!IsDefiniteCoreVertex(v)) {
            auto root_of_u = disjoint_set_ptr->FindRoot(static_cast<uint32_t>(u));
            if (min_cn[j] > 0) {
                min_cn[j] = EvalSimilarity(u, j);
            }
            if (min_cn[j] == SIMILAR) {
                tmp_cluster.emplace_back(cluster_dict[root_of_u], v);
            }
        }
    }
}

void Graph::DSCANFirstPhasePrune() {
    auto prune_start = high_resolution_clock::now();
    {
        auto thread_num = std::thread::hardware_concurrency();
        ThreadPool pool(thread_num);

        auto v_start = 0;
        long deg_sum = 0;
        for (auto v_i = 0; v_i < n; v_i++) {
            deg_sum += degree[v_i];
            if (deg_sum > 64 * 1024) {
                deg_sum = 0;

                pool.enqueue([this](int i_start, int i_end) {
                    for (auto u = i_start; u < i_end; u++) {
                        PruneDetail(u);
                    }
                }, v_start, v_i + 1);
                v_start = v_i + 1;
            }
        }
        pool.enqueue([this](int i_start, int i_end) {
            for (auto u = i_start; u < i_end; u++) {
                PruneDetail(u);
            }
        }, v_start, n);
    }
    auto prune_end = high_resolution_clock::now();
    cout << "1st: prune execution time:" << duration_cast<milliseconds>(prune_end - prune_start).count() << " ms\n";
}

void Graph::DSCANSecondPhaseCheckCore() {
    // check-core 1st phase
    auto find_core_start = high_resolution_clock::now();
    auto thread_num = std::thread::hardware_concurrency();
    {
        ThreadPool pool(thread_num);

        auto v_start = 0;
        long deg_sum = 0;
        for (auto v_i = 0; v_i < n; v_i++) {
            if (core_status_lst[v_i] == UN_KNOWN) {
                deg_sum += degree[v_i];
                if (deg_sum > 32 * 1024) {
                    deg_sum = 0;
                    pool.enqueue([this](int i_start, int i_end) {
                        for (auto i = i_start; i < i_end; i++) { CheckCoreFirstBSP(i); }
                    }, v_start, v_i + 1);
                    v_start = v_i + 1;
                }
            }
        }

        pool.enqueue([this](int i_start, int i_end) {
            for (auto i = i_start; i < i_end; i++) { CheckCoreFirstBSP(i); }
        }, v_start, n);
    }
    auto first_bsp_end = high_resolution_clock::now();
    cout << "2nd: check core first-phase bsp time:"
         << duration_cast<milliseconds>(first_bsp_end - find_core_start).count() << " ms\n";
    PrintMinCnBeauty();

    // check-core 2nd phase
    {
        ThreadPool pool(thread_num);

        auto v_start = 0;
        long deg_sum = 0;
        for (auto v_i = 0; v_i < n; v_i++) {
            if (core_status_lst[v_i] == UN_KNOWN) {
                deg_sum += degree[v_i];
                if (deg_sum > 64 * 1024) {
                    deg_sum = 0;
                    pool.enqueue([this](int i_start, int i_end) {
                        for (auto i = i_start; i < i_end; i++) { CheckCoreSecondBSP(i); }
                    }, v_start, v_i + 1);
                    v_start = v_i + 1;
                }
            }
        }

        pool.enqueue([this](int i_start, int i_end) {
            for (auto i = i_start; i < i_end; i++) { CheckCoreSecondBSP(i); }
        }, v_start, n);
    }

    auto second_bsp_end = high_resolution_clock::now();
    cout << "2nd: check core second-phase bsp time:"
         << duration_cast<milliseconds>(second_bsp_end - first_bsp_end).count() << " ms\n";

    PrintMinCnBeauty();
}

void Graph::DSCANThirdPhaseClusterCore() {
    // trivial: prepare data
    auto tmp_start = high_resolution_clock::now();
    for (auto i = 0; i < n; i++) {
        if (IsDefiniteCoreVertex(i)) { cores.emplace_back(i); }
    }
    cout << "core size:" << cores.size() << "\n";
    auto tmp_end0 = high_resolution_clock::now();
    cout << "3rd: copy time: " << duration_cast<milliseconds>(tmp_end0 - tmp_start).count() << " ms\n";

    // cluster-core 1st phase
    {
        ThreadPool pool(std::thread::hardware_concurrency());

        auto v_start = 0;
        long deg_sum = 0;
        for (auto core_index = 0; core_index < cores.size(); core_index++) {
            deg_sum += degree[cores[core_index]];
            if (deg_sum > 128 * 1024) {
                deg_sum = 0;
                pool.enqueue([this](int i_start, int i_end) {
                    for (auto i = i_start; i < i_end; i++) {
                        auto u = cores[i];
                        ClusterCoreFirstPhase(u);
                    }
                }, v_start, core_index + 1);
                v_start = core_index + 1;
            }
        }

        pool.enqueue([this](int i_start, int i_end) {
            for (auto i = i_start; i < i_end; i++) {
                auto u = cores[i];
                ClusterCoreFirstPhase(u);
            }
        }, v_start, cores.size());
    }

    auto tmp_end = high_resolution_clock::now();
    cout << "3rd: prepare time: " << duration_cast<milliseconds>(tmp_end - tmp_start).count() << " ms\n";

    // cluster-core 2nd phase
    {
        ThreadPool pool(std::thread::hardware_concurrency());

        auto v_start = 0;
        long deg_sum = 0;
        for (auto core_index = 0; core_index < cores.size(); core_index++) {
            deg_sum += degree[cores[core_index]];
            if (deg_sum > 128 * 1024) {
                deg_sum = 0;
                pool.enqueue([this](int i_start, int i_end) {
                    for (auto i = i_start; i < i_end; i++) {
                        auto u = cores[i];
                        ClusterCoreSecondPhase(u);
                    }
                }, v_start, core_index + 1);
                v_start = core_index + 1;
            }
        }

        pool.enqueue([this](int i_start, int i_end) {
            for (auto i = i_start; i < i_end; i++) {
                auto u = cores[i];
                ClusterCoreSecondPhase(u);
            }
        }, v_start, cores.size());
    }
    auto end_core_cluster = high_resolution_clock::now();
    cout << "3rd: core clustering time:" << duration_cast<milliseconds>(end_core_cluster - tmp_start).count()
         << " ms\n";
}

void Graph::MarkClusterMinEleAsId() {
    auto thread_num = std::thread::hardware_concurrency();
    ThreadPool pool(thread_num);
    auto step = max(1u, n / thread_num);
    for (auto outer_i = 0u; outer_i < n; outer_i += step) {
        pool.enqueue([this](ui i_start, ui i_end) {
            for (auto i = i_start; i < i_end; i++) {
                if (IsDefiniteCoreVertex(i)) {
                    int x = disjoint_set_ptr->FindRoot(i);
                    int cluster_min_ele;
                    do {
                        // assume no torn read of cluster_dict[x]
                        cluster_min_ele = cluster_dict[x];
                        if (i >= cluster_dict[x]) {
                            break;
                        }
                    } while (!__sync_bool_compare_and_swap(&cluster_dict[x], cluster_min_ele, i));
                }
            }
        }, outer_i, min(outer_i + step, n));
    }
}

void Graph::DSCANFourthPhaseClusterNonCore() {
    // mark cluster label
    noncore_cluster = std::vector<pair<int, int>>();
    noncore_cluster.reserve(n);

    auto tmp_start = high_resolution_clock::now();
    MarkClusterMinEleAsId();

    auto tmp_next_start = high_resolution_clock::now();
    cout << "4th: marking cluster id cost in cluster-non-core:"
         << duration_cast<milliseconds>(tmp_next_start - tmp_start).count() << " ms\n";

    // cluster non-core 2nd phase
    {
        ThreadPool pool(std::thread::hardware_concurrency());

        auto v_start = 0;
        long deg_sum = 0;
        vector<future<vector<pair<int, int>>>> future_vec;
        for (auto core_index = 0; core_index < cores.size(); core_index++) {
            deg_sum += degree[cores[core_index]];
            if (deg_sum > 32 * 1024) {
                deg_sum = 0;
                future_vec.emplace_back(pool.enqueue([this](int i_start, int i_end) -> vector<pair<int, int>> {
                    auto tmp_cluster = vector<pair<int, int>>();
                    for (auto i = i_start; i < i_end; i++) {
                        auto u = cores[i];
                        ClusterNonCoreDetail(u, tmp_cluster);
                    }
                    return tmp_cluster;
                }, v_start, core_index + 1));
                v_start = core_index + 1;
            }
        }

        future_vec.emplace_back(pool.enqueue([this](int i_start, int i_end) -> vector<pair<int, int>> {
            auto tmp_cluster = vector<pair<int, int>>();
            for (auto i = i_start; i < i_end; i++) {
                auto u = cores[i];
                ClusterNonCoreDetail(u, tmp_cluster);
            }
            return tmp_cluster;
        }, v_start, cores.size()));

        for (auto &future: future_vec) {
            for (auto ele:future.get()) {
                noncore_cluster.emplace_back(ele);
            };
        }
    }

    auto all_end = high_resolution_clock::now();
    cout << "4th: non-core clustering time:" << duration_cast<milliseconds>(all_end - tmp_start).count()
         << " ms\n";
}

void Graph::DSCAN() {
    cout << "new algorithm DSCAN" << endl;
    DSCANFirstPhasePrune();
    PrintMinCnBeauty();

    DSCANSecondPhaseCheckCore();
    PrintMinCnBeauty();


    DSCANThirdPhaseClusterCore();
    PrintMinCnBeauty();

    DSCANFourthPhaseClusterNonCore();
    PrintMinCnBeauty();
}
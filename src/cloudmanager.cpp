/**
 * cloudmanager.cpp
 *
 *    Created on: Sep 4, 2024
 *   Last Update: Sep 9, 2024
 *  Orig. Author: Wade Burch (braernoch.dev@gmail.com)
 *
 *  Copyright 2024 Wade Burch (GPLv3)
 *
 *  This file is part of atomix.
 *
 *  atomix is free software: you can redistribute it and/or modify it under the
 *  terms of the GNU General Public License as published by the Free Software
 *  Foundation, either version 3 of the License, or (at your option) any later
 *  version.
 *
 *  atomix is distributed in the hope that it will be useful, but WITHOUT ANY
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *  FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with
 *  atomix. If not, see <https://www.gnu.org/licenses/>.
 */

#include "cloudmanager.hpp"
#include <ranges>

/*  std::execution (via TBB) and Qt both use the emit keyword, so undef for this file to avoid conflicts  */
#undef emit
#include <execution>


CloudManager::CloudManager() {
    this->cm_pool = std::thread::hardware_concurrency();
    this->cm_loop = 1;
    this->cm_vecs = 0;
}

CloudManager::~CloudManager() {
    cloudPool.purge();
    cloudPool.wait();
    resetManager();
}

void CloudManager::newConfig(AtomixConfig *config) {
    Manager::newConfig(config);

    this->cloudLayerDivisor = cfg.cloudLayDivisor;
    this->cloudResolution = cfg.cloudResolution;
    this->cloudTolerance = cfg.cloudTolerance;
    this->deg_fac = TWO_PI / this->cloudResolution;
    this->cfg.vert = "gpu_harmonics.vert";
}

void CloudManager::receiveCloudMap(harmap *inMap, int numRecipes) {
    this->cloudOrbitals = *inMap;
    this->numOrbitals = numRecipes;
    this->max_n = cloudOrbitals.rbegin()->first;
}

void CloudManager::receiveCloudMapAndConfig(AtomixConfig *config, harmap *inMap, int numRecipes) {
    cm_proc_coarse.lock();

    if (mStatus.hasNone(em::INIT)) {
        newConfig(config);
        receiveCloudMap(inMap, numRecipes);
        initManager();
        mStatus.set(em::INIT);
        cm_proc_coarse.unlock();
        return;
    }
    
    // Check for relevant config changes OR for recipes to require larger radius
    bool widerRadius = (getMaxLayer(config->cloudTolerance, inMap->rbegin()->first, config->cloudLayDivisor) > getMaxLayer(this->cloudTolerance, this->max_n, this->cloudLayerDivisor));
    bool newMap = cloudOrbitals != (*inMap);
    bool newDivisor = (this->cloudLayerDivisor != config->cloudLayDivisor);
    bool newResolution = (this->cloudResolution != config->cloudResolution);
    bool newTolerance = (this->cloudTolerance != config->cloudTolerance);
    bool newCulling = (this->cfg.CloudCull_x != config->CloudCull_x) || (this->cfg.CloudCull_y != config->CloudCull_y) || (this->cfg.CloudCull_rIn != config->CloudCull_rIn) || (this->cfg.CloudCull_rOut != config->CloudCull_rOut);
    bool higherMaxN = (mStatus.hasAny(em::VERT_READY)) && (inMap->rbegin()->first > this->max_n);
    bool configChanged = (newDivisor || newResolution || newTolerance || newCulling);
    bool newVerticesRequired = (newDivisor || newResolution || higherMaxN || widerRadius);

    // Resest or clear if necessary
    if (newVerticesRequired) {
        this->resetManager();
    } else if (newMap) {
        this->clearForNext();
    }

    // Update config
    if (configChanged) {
        this->newConfig(config);
    }
    // Mark for vecsAndMatrices update if map (orbital recipe) has changed
    if (newMap) {
        this->receiveCloudMap(inMap, numRecipes);
        mStatus.set(em::UPD_MATRICES);
    }

    // Re-gen vertices for new config values if necessary
    if (newVerticesRequired) {
        mStatus.clear(em::VERT_READY);
        cm_times[0] = (cm_threading) ? createThreaded() : create();
    }
    // Re-gen PDVs for new map or if otherwise necessary
    if (newVerticesRequired || newMap) {
        mStatus.clear(em::DATA_READY);
        cm_times[1] = (cm_threading) ? bakeOrbitalsThreadedAlt() : bakeOrbitals();
    }
    // Re-cull the indices for tolerance or if otherwise necessary
    if (newVerticesRequired || newMap || newTolerance) {
        mStatus.clear(em::INDEX_GEN);
        cm_times[2] = (cm_threading) ? cullToleranceThreaded() : cullTolerance();
        if (cfg.cpu) expandPDVsToColours();
    }
    // Re-cull the indices for slider position or if otherwise necessary
    if (newVerticesRequired || newMap || newTolerance || newCulling) {
        mStatus.clear(em::INDEX_READY);
        cm_times[3] = (cm_threading) ? cullSliderThreaded() : cullSlider();
    }
    
    // std::cout << "receiveCloudMapAndConfig() -- Functions took:\n";
    // this->printTimes();

    cm_proc_coarse.unlock();
}

void CloudManager::initManager() {
    cm_times[0] = (cm_threading) ? createThreaded() : create();
    cm_times[1] = (cm_threading) ? bakeOrbitalsThreadedAlt() : bakeOrbitals();
    cm_times[2] = (cm_threading) ? cullToleranceThreaded() : cullTolerance();
    if (cfg.cpu) expandPDVsToColours();
    cm_times[3] = (cm_threading) ? cullSliderThreaded() : cullSlider();

    // std::cout << "Init() -- Functions took:\n";
    // this->printTimes();

    mStatus.set(em::UPD_MATRICES);
}

// #pragma GCC push_options
// #pragma GCC optimize ("O3")

double CloudManager::create() {
    assert(mStatus.hasNone(em::VERT_READY));
    system_clock::time_point begin = std::chrono::high_resolution_clock::now();

    this->max_n = cloudOrbitals.rbegin()->first;
    this->opt_max_radius = getMaxLayer(this->cloudTolerance, this->max_n, this->cloudLayerDivisor);
    int phi_max_local = this->cloudResolution >> 1;
    int theta_max_local = this->cloudResolution;
    double deg_fac_local = this->deg_fac;

    this->pixelCount = opt_max_radius * theta_max_local * phi_max_local;
    vec3 pos = vec3(0.0f);

    // auto beginInner = std::chrono::high_resolution_clock::now();
    allVertices.reserve(pixelCount);
    for (int k = 1; k <= opt_max_radius; k++) {
        double radius = static_cast<double>(k) / this->cloudLayerDivisor;

        for (int i = 0; i < theta_max_local; i++) {
            double theta = i * deg_fac_local;
            for (int j = 0; j < phi_max_local; j++) {
                double phi = j * deg_fac_local;

                if (cfg.cpu) {
                    pos.x = radius * sin(phi) * sin(theta);
                    pos.y = radius * cos(phi);
                    pos.z = radius * sin(phi) * cos(theta);
                } else {
                    pos.x = (float) radius;
                    pos.y = (float) theta;
                    pos.z = (float) phi;
                }

                allVertices.push_back(pos);
            }
        }
    }
    // auto endInner = std::chrono::high_resolution_clock::now();
    // auto createTime = std::chrono::duration<double, std::milli>(endInner - beginInner).count();
    // std::cout << "Create Inner Loop took: " << createTime << " ms" << std::endl;

    dataStaging.insert(dataStaging.end(), pixelCount, 0.0);
    allData.insert(allData.end(), pixelCount, 0.0f);
    idxCulledTolerance.reserve(pixelCount);

    wavefuncNorms(MAX_SHELLS);

    mStatus.advance(em::INIT, em::VERT_READY);
    genVertexArray();
    system_clock::time_point end = std::chrono::high_resolution_clock::now();
    return (std::chrono::duration<double, std::milli>(end - begin).count());
}

double CloudManager::createThreaded() {
    assert(mStatus.hasNone(em::VERT_READY));
    cm_proc_fine.lock();
    system_clock::time_point begin = std::chrono::high_resolution_clock::now();

    this->max_n = cloudOrbitals.rbegin()->first;
    int div_local = this->cloudLayerDivisor;
    this->opt_max_radius = getMaxLayer(this->cloudTolerance, this->max_n, div_local);
    int theta_max_local = this->cloudResolution;
    int phi_max_local = this->cloudResolution >> 1;
    int layer_size = theta_max_local * phi_max_local;
    double deg_fac_local = this->deg_fac;
    this->pixelCount = this->opt_max_radius * theta_max_local * phi_max_local;
    bool isGPU = !cfg.cpu;

    /*  Memory -- Begin --- This memory-carving portion takes 94% of create() total time  */
    // auto beginInner = std::chrono::high_resolution_clock::now();
    allVertices.reserve(pixelCount);
    dataStaging.reserve(pixelCount);
    allData.reserve(pixelCount);
    idxCulledTolerance.reserve(pixelCount);

    allVertices.assign(pixelCount, vec3(0.0f));
    dataStaging.assign(pixelCount, 0.0);
    allData.assign(pixelCount, 0.0f);

    wavefuncNorms(MAX_SHELLS);
    // auto endInner = std::chrono::high_resolution_clock::now();
    // auto createTime = std::chrono::duration<double, std::milli>(endInner - beginInner).count();
    // std::cout << "Create Inner Loop took: " << createTime << " ms" << std::endl;

    /*  Compute -- Begin --- This compute portion takes only 6% of create() total time  */
    // auto beginInner = std::chrono::high_resolution_clock::now();
    vec3 *start = &this->allVertices.at(0);
    std::for_each(std::execution::par_unseq, allVertices.begin(), allVertices.end(),
        [layer_size, phi_max_local, deg_fac_local, div_local, start, isGPU](glm::vec3 &gVector){
            int i = &gVector - start;
            int layer = (i / layer_size) + 1;
            int layer_pos = i % layer_size;
            float theta = (layer_pos / phi_max_local) * deg_fac_local;
            float phi = (layer_pos % phi_max_local) * deg_fac_local;
            float radius = static_cast<float>(layer) / div_local;

            if (isGPU) {
                gVector.x = radius;
                gVector.y = theta;
                gVector.z = phi;
            } else {
                gVector.x = radius * sin(phi) * sin(theta);
                gVector.y = radius * cos(phi);
                gVector.z = radius * sin(phi) * cos(theta);
            }
        });
    // auto endInner = std::chrono::high_resolution_clock::now();
    // auto createTime = std::chrono::duration<double, std::milli>(endInner - beginInner).count();
    // std::cout << "Create Inner Loop took: " << createTime << " ms" << std::endl;

    /*  Exit  */
    mStatus.set(em::VERT_READY);
    genVertexArray();
    system_clock::time_point end = std::chrono::high_resolution_clock::now();
    cm_proc_fine.unlock();
    return (std::chrono::duration<double, std::milli>(end - begin).count());
}

void CloudManager::genOrbital(int n, int l, int m_l, double weight) {
    double deg_fac_local = this->deg_fac;
    int phi_max_local = this->cloudResolution >> 1;
    int theta_max_local = this->cloudResolution;
    this->opt_max_radius = getMaxLayer(this->cloudTolerance, this->max_n, this->cloudLayerDivisor);
    double orbNorm = this->norm_constY[DSQ(l, m_l)];

    for (int k = 1; k <= opt_max_radius; k++) {
        double radius = static_cast<double>(k) / this->cloudLayerDivisor;
        double R = wavefuncRadial(n, l, radius);
        for (int i = 0; i < theta_max_local; i++) {
            double theta = i * deg_fac_local;
            std::complex<double> orbExp = wavefuncAngExp(m_l, theta);
            for (int j = 0; j < phi_max_local; j++) {
                double phi = j * deg_fac_local;
                int localCount = ((k-1) * theta_max_local * phi_max_local) + (i * phi_max_local) + j;

                double orbLeg = wavefuncAngLeg(l, m_l, phi);
                std::complex<double> Y = orbExp * orbNorm * orbLeg;
                double pdv = wavefuncPDV(R * Y, radius, l);

                dataStaging[localCount] += (pdv * weight);
            }
        }
    }
}

double CloudManager::bakeOrbitals() {
    assert(mStatus.hasNone(em::INIT) && mStatus.hasFirstNotLast(em::VERT_READY, em::DATA_READY));
    system_clock::time_point begin = std::chrono::high_resolution_clock::now();

    // Iterate through stored recipes, grouped by N
    for (auto const &[key, val] : cloudOrbitals) {
        for (auto const &v : val) {
            genOrbital(key, v.x, v.y, (v.z / static_cast<double>(numOrbitals)));
        }
    }

    // End: check actual max value of accumulated vector
    this->allPDVMaximum = *std::max_element(dataStaging.begin(), dataStaging.end());
    // mStatus.set(em::DATA_GEN);
    system_clock::time_point end = std::chrono::high_resolution_clock::now();
    return (std::chrono::duration<double, std::milli>(end - begin).count());
}

double CloudManager::bakeOrbitalsThreaded() {
    assert(mStatus.hasFirstNotLast(em::VERT_READY, em::DATA_READY));
    cm_proc_fine.lock();
    system_clock::time_point begin = std::chrono::high_resolution_clock::now();

    // this->printRecipes();

    /*  This section contains 62%-98% of the total execution time of cloud generation! */
    const double totalRecipes = static_cast<double>(this->numOrbitals);
    const double *vecStaging = &this->dataStaging[0];
    const vVec3 *vecVertices = &this->allVertices;
    const harmap *mapRecipes = &this->cloudOrbitals;
    std::unordered_map<int, double> *ncR = &this->norm_constR;
    std::unordered_map<int, double> *ncY = &this->norm_constY;
    std::for_each(std::execution::par_unseq, dataStaging.begin(), dataStaging.end(),
        [totalRecipes, vecStaging, vecVertices, mapRecipes, ncR, ncY](double &item){
            std::ptrdiff_t i = &item - vecStaging;
            double final_pdv = 0;
            double theta = (*vecVertices)[i].x;
            double phi = (*vecVertices)[i].y;
            double radius = (*vecVertices)[i].z;

            for (auto const &[key, val] : (*mapRecipes)) {
                int n = key;
                for (auto const &v : val) {
                    int l = v.x;
                    int m_l = v.y;
                    double weight = v.z / totalRecipes;

                    double rho = 2.0 * radius / static_cast<double>(n);
                    double rhol = 1.0;
                    int l_times = l;
                    while (l_times-- > 0) {
                        rhol *= rho;
                    }
                    double R = lagp((n - l - 1), ((l << 1) + 1), rho) * rhol * exp(-rho/2.0) * (*ncR)[DSQ(n, l)];
                    std::complex<double> Y = exp(std::complex<double>{0,1} * static_cast<double>(m_l) * theta) * legp(l, abs(m_l), cos(phi)) * (*ncY)[DSQ(l, m_l)];
                    std::complex<double> Psi = R * Y;

                    double pdv = (std::conj(Psi) * Psi).real() * radius * radius;;
                    final_pdv += pdv * weight;
                }
            }
            item = final_pdv;
        });
    
    // End: check actual max value of accumulated vector
    this->allPDVMaximum = *std::max_element(std::execution::par, dataStaging.cbegin(), dataStaging.cend());

    // End: Populate allData with normalized PDVs [** as FLOATS **], leaving dataStaging untouched
    double pdvMax = this->allPDVMaximum;
    std::transform(std::execution::par_unseq, dataStaging.cbegin(), dataStaging.cend(), allData.begin(),
        [pdvMax](const double &item){
            return static_cast<float>(item / pdvMax);
        });
    
    // End: dataStaging is not needed after this
    dataStaging.clear();

    mStatus.set(em::DATA_READY);
    genDataBuffer();

    system_clock::time_point end = std::chrono::high_resolution_clock::now();
    cm_proc_fine.unlock();
    return (std::chrono::duration<double, std::milli>(end - begin).count());
}

double CloudManager::bakeOrbitalsThreadedAlt() {
    assert(mStatus.hasFirstNotLast(em::VERT_READY, em::DATA_READY));
    cm_proc_fine.lock();
    system_clock::time_point begin = std::chrono::high_resolution_clock::now();

    /*  Prep -- Threading  */
    std::vector<BS::multi_future<void> *> allRecipeFutures;
    std::vector<std::vector<double> *> vec_top_threads;
    dvec *thread_vec = nullptr;
    int thread_loop_limit = this->cm_loop;
    uint thread_vec_limit = this->cm_vecs;
    int vecs_to_fill = numOrbitals - 1;
    uint num_top_vectors = (numOrbitals <= thread_vec_limit) ? vecs_to_fill : thread_vec_limit;
    bool no_vecs = !num_top_vectors;
    for (uint top = 0; top < num_top_vectors; top++) {
        vec_top_threads.push_back(new std::vector<double>);
        (*vec_top_threads[top]).assign(this->pixelCount, 0.0);
    }
    uint recipe_idx = 0;
    uint clearing_vec = 0;
    int clearing_fut = 0;
    dvec *dataStagingPtr = &this->dataStaging;
    // uint col_max = this->pixelCount;

    /*  Prep -- Compute  */
    double deg_fac_local = this->deg_fac;
    int div_local = this->cloudLayerDivisor;
    int phi_max_local = this->cloudResolution >> 1;
    int theta_max_local = this->cloudResolution;
    this->opt_max_radius = this->getMaxRadius(this->cloudTolerance, this->max_n) * div_local + 1;
    int opt_max_radius_local = this->opt_max_radius;

    /*  Compute -- Begin
        This section contains 62%-98% of the total execution time of cloud generation,
        which can easily scale into Ne+1 minutes for high resolutions. For that reason,
        I'm unrolling all the pretty functions that go into this calc (hyperoptimization).
    */
    // system_clock::time_point inner_begin = std::chrono::high_resolution_clock::now();
    // Recipe Loop
    for (auto const &[key, val] : cloudOrbitals) {
        for (auto const &v : val) {
            int n = key;
            int l = v.x;
            int m_l = v.y;
            float weight = static_cast<float>(v.z) / static_cast<float>(this->numOrbitals);
            double angNorm = this->norm_constY[DSQ(l, m_l)];
            double radNorm = this->norm_constR[DSQ(n, l)];
            uint top_idx = (num_top_vectors) ? (recipe_idx % num_top_vectors) : 0;
            // std::cout << "Recipe: (" << n << l << m_l << ")" << std::endl;

            if ((++recipe_idx == numOrbitals) || no_vecs) {
                thread_vec = dataStagingPtr;                // 6/6
            } else {
                thread_vec = vec_top_threads[top_idx];   // 1/6= 0, 2/6= 1, 3/6= 2, 4/6= 3, 5/6= 0
            }
            
            // Store all futures from this recipe
            BS::multi_future<void> *recipeFutures = new BS::multi_future<void>;
            allRecipeFutures.push_back(recipeFutures);

            // Divide up the Layers loop into chunks and spawn threads to handle those chunks, from the thread pool
            int top_start = 1, top_end = 1;
            while (top_end < opt_max_radius_local) {
                if (opt_max_radius_local - top_end >= thread_loop_limit) {
                    top_end = top_start + thread_loop_limit;
                } else {
                    top_end = opt_max_radius_local;
                }

                // Spawn thread for each chunk
                recipeFutures->push_back(cloudPool.submit_task(
                [n, l, m_l, weight, radNorm, angNorm, div_local, theta_max_local, phi_max_local, deg_fac_local, thread_vec, top_start, top_end](){

                    // Layer Loop
                    for (int k = top_start; k < top_end; k++) {
                        double radius = static_cast<double>(k) / div_local;
                        int layer_idx = (k-1) * theta_max_local * phi_max_local;
                        
                        // Radial wavefunc
                        double rho = 2.0 * radius / static_cast<double>(n);
                        double rhol = 1.0;
                        // (factorial subroutine)
                        for (int l_times = l; l_times > 0; l_times--) {
                            rhol *= rho;
                        }
                        double R = lagp((n - l - 1), ((l << 1) + 1), rho) * rhol * exp(-rho/2.0) * radNorm;
                        
                        // Theta Loop
                        for (int i = 0; i < theta_max_local; i++) {
                            double theta = i * deg_fac_local;
                            int theta_idx = i * phi_max_local;
                            std::complex<double> angExp = exp(std::complex<double>{0,1} * (m_l * theta));
                            
                            // Phi Loop
                            for (int j = 0; j < phi_max_local; j++) {
                                double phi = j * deg_fac_local;

                                // Angular wavefunc
                                double orbLeg = legp(l, abs(m_l), cos(phi));
                                std::complex<double> Psi = R * angExp * angNorm * orbLeg;
                                double pdv = (std::conj(Psi) * Psi).real() * radius * radius;

                                int localCount = layer_idx + theta_idx + j;
                                (*thread_vec)[localCount] += (pdv * weight);
                            } // End of Phi Loop
                        } // End of Theta Loop
                    } // End of Layer Loop (layers per chunk)
                })); // End of lambda (thread loop -- the chunk)
                
                top_start = top_end;
            } // End of Layer Loop (whole)

            // If this is the last recipe, it was assigned the dataStaging vector, so wait for that and then collapse all other spawned vectors
            if (no_vecs) {
                allRecipeFutures.back()->wait();
            } else if (recipe_idx == numOrbitals) {
                allRecipeFutures.back()->wait();
                while (clearing_fut < vecs_to_fill) {
                    /* BS::multi_future<void> futureCollapse;
                    uint col_start = 0, col_end = 0;
                    dvec *col_vec = vec_top_threads[clearing_vec]; */
                    (*allRecipeFutures[clearing_fut++]).wait();
                    double *d_start = &(*vec_top_threads[clearing_vec])[0];
                    std::for_each(std::execution::par_unseq, (*vec_top_threads[clearing_vec]).cbegin(), (*vec_top_threads[clearing_vec]).cend(),
                        [d_start, dataStagingPtr](const double &item){
                            uint idx = &item - d_start;
                            (*dataStagingPtr)[idx] += item;
                        });
                    /* for (uint64_t c = 0; c < col_max; c += thread_loop_limit) {
                        col_start = c;
                        col_end = (c + thread_loop_limit > col_max) ? thread_loop_limit : c + thread_loop_limit;
                        
                        futureCollapse.push_back(cloudPool.submit_task(
                            [dataStagingPtr, col_vec, col_start, col_end](){
                                for (int c = col_start; c < col_end; c++) {
                                    (*dataStagingPtr)[c] += (*col_vec)[c];
                                }
                            }, BS::pr::highest));
                    }
                    futureCollapse.wait(); */
                    if (++clearing_vec >= num_top_vectors) {
                        clearing_vec = 0;
                    }
                }
            // If this is not the last recipe, but we have filled all spawned vectors, collapse and reset first-filled for next round of threads
            } else if (recipe_idx >= num_top_vectors) {
                /* BS::multi_future<void> futureCollapse;
                uint col_start = 0, col_end = 0;
                dvec *col_vec = vec_top_threads[clearing_vec]; */
                allRecipeFutures[clearing_fut++]->wait();
                double *d_start = &(*vec_top_threads[clearing_vec])[0];
                std::for_each(std::execution::par_unseq, (*vec_top_threads[clearing_vec]).cbegin(), (*vec_top_threads[clearing_vec]).cend(),
                    [d_start, dataStagingPtr](const double &item){
                        uint idx = &item - d_start;
                        (*dataStagingPtr)[idx] += item;
                    });
                /* for (uint64_t c = 0; c < col_max; c += thread_loop_limit) {
                    col_start = c;
                    col_end = (c + thread_loop_limit > col_max) ? thread_loop_limit : c + thread_loop_limit;
                
                    futureCollapse.push_back(cloudPool.submit_task(
                        [dataStagingPtr, col_vec, col_start, col_end](){
                            for (int c = col_start; c < col_end; c++) {
                                (*dataStagingPtr)[c] += (*col_vec)[c];
                            }
                        }, BS::pr::highest));
                }
                futureCollapse.wait(); */
                (*vec_top_threads[clearing_vec]).assign(this->pixelCount, 0.0);
                if (++clearing_vec >= num_top_vectors) {
                    clearing_vec = 0;
                }
            }
        } // End of (l, m_l) loop
    } // End of (n) loop

    /*  Compute -- Post-processing  */
    // Check actual max value of accumulated vector
    this->allPDVMaximum = *std::max_element(std::execution::par, dataStaging.begin(), dataStaging.end());

    // Populate allData with normalized PDVs [** as FLOATS **], leaving dataStaging untouched
    double pdvMax = this->allPDVMaximum;
    std::transform(std::execution::par_unseq, dataStaging.cbegin(), dataStaging.cend(), allData.begin(),
        [pdvMax](const double &item){
            return static_cast<float>(item / pdvMax);
        });
    
    /*  Cleanup  */
    for (auto &v : vec_top_threads) {
        delete v;
    }
    for (auto &f : allRecipeFutures) {
        delete f;
    }
    dataStaging.clear();
    
    /*  Exit  */
    mStatus.set(em::DATA_READY);
    genDataBuffer();
    system_clock::time_point end = std::chrono::high_resolution_clock::now();
    cm_proc_fine.unlock();
    return (std::chrono::duration<double, std::milli>(end - begin).count());
}

double CloudManager::cullTolerance() {
    // assert(mStatus.hasFirstNotLast(em::DATA_READY, (em::INDEX_GEN | em::INDEX_READY)));
    system_clock::time_point begin = std::chrono::high_resolution_clock::now();

    std::fill(allData.begin(), allData.end(), 0.0);
    idxCulledTolerance.clear();

    for (uint p = 0; p < this->pixelCount; p++) {
        double new_val = dataStaging[p] / this->allPDVMaximum;
        if (new_val > this->cloudTolerance) {
            allData[p] = static_cast<float>(new_val);
            idxCulledTolerance.push_back(p);
        }
    }
    allIndices.reserve(idxCulledTolerance.size());

    mStatus.set(em::DATA_READY);
    system_clock::time_point end = std::chrono::high_resolution_clock::now();
    return (std::chrono::duration<double, std::milli>(end - begin).count());
}

double CloudManager::cullToleranceThreaded() {
    assert(mStatus.hasFirstNotLast(em::DATA_READY, em::INDEX_GEN));
    cm_proc_fine.lock();
    system_clock::time_point begin = std::chrono::high_resolution_clock::now();
    idxCulledTolerance.resize(this->dataCount);
    idxCulledTolerance.assign(this->dataCount, 0);

    /* The joys of debugging segfaults and exceptions in asynchronous, non-static, Filter and Map functions called via lambdas. */

    // Populate idxCulledTolerance with visible indices based on Tolerance, leaving allData untouched for future Tolerance tests
    const float *vecStart = &allData[0];
    const float tolerance_local = this->cloudTolerance;
    std::transform(std::execution::par_unseq, allData.cbegin(), allData.cend(), idxCulledTolerance.begin(),
        [tolerance_local, vecStart](const float &item){
            uint idx = &item - vecStart;
            return (item > tolerance_local) ? idx : (uint) 0;
        });
    
    // Isolate unused indices for easy removal, then remove them
    auto itEnd = std::remove_if(std::execution::par_unseq, idxCulledTolerance.begin(), idxCulledTolerance.end(),
        [](uint &item){
            return !item;
        });
    // idxCulledTolerance.resize(std::distance(idxCulledTolerance.begin(), itEnd));
    idxCulledTolerance.resize((itEnd - idxCulledTolerance.begin()));
    
    // Our model now displays cm_pixels count of indices/vertices unless culled by slider
    this->cm_pixels = idxCulledTolerance.size();
    allIndices.reserve(this->cm_pixels);

    /*  Exit  */
    mStatus.set(em::INDEX_GEN);
    system_clock::time_point end = std::chrono::high_resolution_clock::now();
    cm_proc_fine.unlock();
    return (std::chrono::duration<double, std::milli>(end - begin).count());
}

double CloudManager::expandPDVsToColours() {
    allColours.resize(allVertices.size());
    allColours.assign(allVertices.size(), vec3(0.0f, 0.0f, 0.0f));

    vec3 colours[11] = {
        vec3(2.0f, 0.0f, 2.0f),      // [0-9%] -- Magenta
        vec3(0.0f, 0.0f, 1.5f),      // [10-19%] -- Blue
        vec3(0.0f, 0.5f, 1.0f),      // [20-29%] -- Cyan-Blue
        vec3(0.0f, 1.0f, 0.5f),      // [30-39%] -- Cyan-Green
        vec3(0.0f, 1.0f, 0.0f),      // [40-49%] -- Green
        vec3(1.0f, 1.0f, 0.0f),      // [50-59%] -- Yellow
        vec3(1.0f, 1.0f, 0.0f),      // [60-69%] -- Yellow
        vec3(1.0f, 0.0f, 0.0f),      // [70-79%] -- Red
        vec3(1.0f, 0.0f, 0.0f),      // [80-89%] -- Red
        vec3(1.0f, 1.0f, 1.0f),      // [90-99%] -- White
        vec3(1.0f, 1.0f, 1.0f)       // [100%] -- White
    };
    const uint *vecStart = &idxCulledTolerance[0];
    const float *dataStart = &allData[0];
    vec3 *vecColours = &allColours[0];
    uint64_t idx = 0;
    uint64_t *idxPtr = &idx;
    std::for_each(std::execution::par, idxCulledTolerance.begin(), idxCulledTolerance.end(),
        [vecStart, dataStart, colours, vecColours, idxPtr](uint &item){
            float pdv = dataStart[item];
            uint colourIdx = uint(pdv * 10.0f);
            vec3 pdvColour = colours[colourIdx] * pdv;

            vecColours[item] = pdvColour;
        });

    genColourBuffer();
    return 0.0;
}

double CloudManager::cullSlider() {
    assert(mStatus.hasFirstNotLast(em::INDEX_GEN, em::INDEX_READY));
    system_clock::time_point begin = std::chrono::high_resolution_clock::now();
    if (allIndices.size()) {
        allIndices.clear();
    }

    if (!(this->cfg.CloudCull_x || this->cfg.CloudCull_y)) {
        std::copy(idxCulledTolerance.cbegin(), idxCulledTolerance.cend(), std::back_inserter(allIndices));
    } else {
        uint layer_size = 0, culled_theta_all = 0, phi_size = 0, culled_phi_b = 0, culled_phi_f = 0;
        layer_size = (this->cloudResolution * this->cloudResolution) >> 1;
        culled_theta_all = static_cast<uint>(ceil(layer_size * this->cfg.CloudCull_x));
        phi_size = this->cloudResolution >> 1;
        culled_phi_b = static_cast<uint>(ceil(phi_size * (1.0f - this->cfg.CloudCull_y))) + phi_size;
        culled_phi_f = static_cast<uint>(ceil(phi_size * this->cfg.CloudCull_y));
        uint idxEnd = idxCulledTolerance.size();

        for (uint i = 0; i < idxEnd; i++) {
            uint phi_pos = idxCulledTolerance[i] % phi_size;
            bool cull_theta = ((idxCulledTolerance[i] % layer_size) <= culled_theta_all);
            bool cull_theta_phi = (phi_pos <= phi_size);
            bool cull_phi_front = (phi_pos <= culled_phi_f);
            bool cull_phi_back = (phi_pos >= culled_phi_b);

            if (!((cull_theta && cull_theta_phi) || (cull_phi_front || cull_phi_back))) {
                idxCulledSlider.push_back(idxCulledTolerance[i]);
            }
        }

        std::copy(idxCulledTolerance.begin(), idxCulledTolerance.end(), std::back_inserter(allIndices));
    }

    mStatus.set(em::INDEX_READY);
    genIndexBuffer();
    system_clock::time_point end = std::chrono::high_resolution_clock::now();
    return (std::chrono::duration<double, std::milli>(end - begin).count());
}

double CloudManager::cullSliderThreaded() {
    assert(mStatus.hasFirstNotLast(em::INDEX_GEN, em::INDEX_READY));
    cm_proc_fine.lock();
    system_clock::time_point begin = std::chrono::high_resolution_clock::now();

    bool visible = !bool(int(this->cfg.CloudCull_x) + int(this->cfg.CloudCull_y) + int(this->cfg.CloudCull_rIn) + int(this->cfg.CloudCull_rOut));
    bool rin = (this->cfg.CloudCull_rIn);
    bool rout = (this->cfg.CloudCull_rOut);
    bool radial = (rin || rout);
    bool angular = ((this->cfg.CloudCull_x) || (this->cfg.CloudCull_y));
    bool untouched  = !(angular || radial);

    uint radial_layers = this->opt_max_radius;
    if (radial) {
        radial_layers *= (rin) ? (1.0f - this->cfg.CloudCull_rIn) : this->cfg.CloudCull_rOut;
    }
    uint64_t rad_threshold = radial_layers * this->cloudResolution * (this->cloudResolution >> 1);

    if (visible) {
        if (untouched) {
            //  Default -- X/Y sliders are not culling, so copy idxCulledTolerance directly to allIndices! 
            allIndices.resize(this->cm_pixels);
            allIndices.assign(this->cm_pixels, 0);
            std::copy(std::execution::par, idxCulledTolerance.cbegin(), idxCulledTolerance.cend(), allIndices.begin());

            /* if (radial) {
                if (rin) {
                    this->indexOffset = 0;
                    this->indexCount = rad_threshold;
                } else {
                    this->indexOffset = rad_threshold;
                    this->indexCount = allIndices.size() - rad_threshold;
                }
                mStatus.condSet(em::UPD_IDXOFF, mStatus.hasAll(em::INIT));
            } */
        } else {
            //  Other -- X/Y sliders ARE culling, so count number of unculled vertices, resize allIndices, and then copy unculled vertices.  
            uint layer_size = 0, theta_size = 0, culled_theta_all = 0, phi_size = 0, culled_phi_f = 0, culled_phi_b = 0;
            float phi_front_pct = 0.0f, phi_back_pct = 0.0f;
            layer_size = (this->cloudResolution * this->cloudResolution) >> 1;
            culled_theta_all = static_cast<uint>(ceil(layer_size * this->cfg.CloudCull_x));
            theta_size = this->cloudResolution;
            phi_size = this->cloudResolution >> 1;
            if (this->cfg.CloudCull_y > 0.50f) {
                phi_front_pct = 1.0f;
                phi_back_pct = (this->cfg.CloudCull_y - 0.50f) * 2.0f;
            } else {
                phi_front_pct = this->cfg.CloudCull_y * 2.0f;
                phi_back_pct = 0.0f;
            }
            culled_phi_f = static_cast<uint>(ceil(phi_size * phi_front_pct));
            culled_phi_b = phi_size - static_cast<uint>(ceil(phi_size * phi_back_pct));

            // Define lambda for multi-use
            auto lambda_cull = [layer_size, theta_size, culled_theta_all, phi_size, culled_phi_f, culled_phi_b, rad_threshold, rin, rout](const uint &item){
                uint layer_pos = (item % layer_size);
                uint theta_pos = layer_pos / phi_size;
                uint phi_pos = item % phi_size;
                bool culled_theta = (layer_pos <= culled_theta_all);
                bool culled_theta_phis = (phi_pos <= phi_size);
                bool culled_phi_front = (phi_pos <= culled_phi_f);
                bool culld_phi_back = (phi_pos >= culled_phi_b);
                bool culled_phi_thetas_front = (theta_pos <= phi_size);   // phi_size here is theta_size/2
                bool culled_phi_thetas_back = (theta_pos > phi_size);   // phi_size here is theta_size/2
                bool culled_radial_in = rin && (item > rad_threshold);
                bool culled_radial_out = rout && (item < rad_threshold);

                bool theta_culled = (culled_theta && culled_theta_phis);
                bool phi_culled = (culled_phi_front && culled_phi_thetas_front) || (culld_phi_back && culled_phi_thetas_back);
                bool radial_culled = (culled_radial_in || culled_radial_out);

                return !(theta_culled || phi_culled || radial_culled);
            };

            // Count unculled vertices
            uint pix_final = std::count_if(std::execution::par_unseq, idxCulledTolerance.cbegin(), idxCulledTolerance.cend(), lambda_cull);

            // Resize allIndices. ***Note: resize does NOT change capacity, so full size is still reserved!
            allIndices.resize(pix_final);
            allIndices.assign(pix_final, 0);

            // Copy only unculled vertices to allIndices
            std::copy_if(std::execution::par_unseq, idxCulledTolerance.cbegin(), idxCulledTolerance.cend(), allIndices.begin(), lambda_cull);
        }
    }

    /*  Exit  */
    mStatus.set(em::INDEX_READY);
    genIndexBuffer();
    this->indexCount *= uint64_t(visible);
    system_clock::time_point end = std::chrono::high_resolution_clock::now();
    cm_proc_fine.unlock();
    return std::chrono::duration<double, std::milli>(end - begin).count();
}

void CloudManager::update([[maybe_unused]] double time) {
    //TODO implement for CPU updates over time
}

void CloudManager::cloudTest(int n_max) {
    int idx = 0;

    std::vector<int> cloudMap;

    for (int n = n_max; n > 0; n--) {
        for (int l = n-1; l >= 0; l--) {
            std::cout << std::setw(3) << idx++ << ")   (" << n << " , " << l << ")\n        ";
            for (int m_l = l; m_l >= -l; m_l--) {
                std::cout << m_l << ((m_l == -l) ? "" : ", ");
                cloudMap.push_back( ((n<<2)*(n<<2)) + ((l<<1) * (l<<1)) + m_l );
            }
            std::cout << std::endl;
        }
    }
    std::cout << std::endl;

    for (const auto &i : cloudMap) {
        std::cout << i << ",";
    }
    std::cout << std::endl;

    std::sort(cloudMap.begin(), cloudMap.end());
    auto it = std::adjacent_find(cloudMap.begin(), cloudMap.end());
    std::cout << "Duplicates " << ((it == cloudMap.end()) ? "NOT found!" : "found.") << std::endl;
}

void CloudManager::cloudTestCSV() {
    for (int n = 1; n <= 8; n++) {
        for (int l = 0; l <= n-1; l++) {
            for (int m_l = 0; m_l <= l; m_l++) {
                std::cout << n << l << m_l;

                for (int k = 1; k <= 250; k++) {
                    double max_pdv = 0;
                    int steps_local = this->cloudResolution;
                    double deg_fac_local = this->deg_fac;
                    double orbNorm = this->norm_constY[DSQ(l, m_l)];
                    double R = wavefuncRadial(n, l, k);

                    for (int i = 0; i < steps_local; i++) {
                        double theta = i * deg_fac_local;
                        std::complex<double> orbExp = wavefuncAngExp(m_l, theta);
                        for (int j = 0; j < steps_local; j++) {
                            double phi = j * deg_fac_local;

                            double orbLeg = wavefuncAngLeg(l, m_l, phi);
                            std::complex<double> Y = orbExp * orbNorm * orbLeg;
                            double pdv = wavefuncPDV(R * Y, k, l);

                            if (pdv > max_pdv) {
                                max_pdv = pdv;
                            }
                        }
                    }

                    std::cout << "," << max_pdv;
                }
                std::cout << "\n";
            }

        }
    }
    std::cout << std::endl;

}

int64_t CloudManager::fact(int n) {
    int64_t prod = n ?: 1;
    // int orig = n;

    while (n > 2) {
        prod *= --n;
    }

    /* if (orig && prod > std::numeric_limits<int>::max()) {
        std::cout << orig << "! was too large for [int] type." << std::endl;
        orig = 0;
    } */

    return prod;
}

double CloudManager::wavefuncRadial(int n, int l, double r) {
    double rho = 2.0 * r / static_cast<double>(n);

    double laguerre = lagp((n - l - 1), ((l << 1) + 1), rho);
    double expFunc = exp(-rho/2.0);
    double rhol = pow(rho, l);

    return laguerre * rhol * expFunc * this->norm_constR[DSQ(n, l)];
}

std::complex<double> CloudManager::wavefuncAngular(int l, int m_l, double theta, double phi) {
    using namespace std::complex_literals;
    std::complex<double> ibase = 1i;

    double legendre = legp(l, abs(m_l), cos(phi));
    ibase *= m_l * theta;
    std::complex<double> expFunc = exp(ibase);

    return expFunc * legendre * this->norm_constY[DSQ(l, m_l)];
}

std::complex<double> CloudManager::wavefuncAngExp(int m_l, double theta) {
    using namespace std::complex_literals;
    std::complex<double> ibase = 1i;
    return exp(ibase * (m_l * theta));
}

double CloudManager::wavefuncAngLeg(int l, int m_l, double phi) {
    return legp(l, abs(m_l), cos(phi));
}

std::complex<double> CloudManager::wavefuncPsi(double radial, std::complex<double> angular) {
    return radial * angular;
}

double CloudManager::wavefuncRDP(double R, double r, [[maybe_unused]] int l) {
    double factor = r * r;

    /* if (!l) {
        factor *= 4.0 * M_PI;
    } */

    return R * R * factor;
}

double CloudManager::wavefuncPDV(std::complex<double> Psi, double r, [[maybe_unused]] int l) {
    double factor = r * r;

    /* if (!l) {
        factor *= 0.4 * M_PI;
    } */

    /* std::complex<double> term1 = std::conj(Psi);
    std::complex<double> term2 = term1 * Psi;
    double term3 = term2.real(); */

    return (std::conj(Psi) * Psi).real() * factor;
}

double CloudManager::wavefuncPsi2(int n, int l, int m_l, double r, double theta, double phi) {
    double factor = r * r;

    double R = wavefuncRadial(n, l, r);
    std::complex<double> Y = wavefuncAngular(l, m_l, theta, phi);
    std::complex<double> Psi = R * Y;

    return (std::conj(Psi) * Psi).real() * factor;
}

void CloudManager::wavefuncNorms(int max_n) {
    int max_l = max_n - 1;

    for (int n = max_n; n > 0; n--) {
        double rho_r = (2.0 * this->atomZ) / n;
        for (int l = n-1; l >= 0; l--) {
            int nl1 = n - l - 1;
            int key = DSQ(n, l);

            double value = pow(rho_r, (3.0/2.0)) * sqrt(fact(nl1) / (2.0 * n * fact(n+l)));
            this->norm_constR[key] = value;
        }
    }
    for (int l = max_l; l >= 0; l--) {
        int l21 = 2 * l + 1;
        for (int m_l = -l; m_l <= l; m_l++) {
            int magM = abs(m_l);
            int key = DSQ(l, m_l);

            double value = sqrt((l21 / (4.0 * M_PI)) * (fact(l - magM) / static_cast<double>(fact(l + magM))));
            this->norm_constY[key] = value;
        }
    }
}

/* void CloudManager::RDPtoColours() {
    // genRDPs();
    // TODO implement RDP-to-Colour conversion for CPU



    for (auto f : max_rads) {
        std::cout << f << ", ";
    }
    std::cout << std::endl;
    for (auto f : max_RDPs) {
        std::cout << f << ", ";
    }
    std::cout << std::endl;

    std::vector<float>::iterator fIt = std::max_element(allData.begin(), allData.end());
    int idx = std::distance(allData.begin(), fIt);
    std::cout << "Max value " << *fIt << " at index " << idx << std::endl;

} */

/* void CloudManager::resetUpdates() {
    mStatus.reset();
} */

void CloudManager::clearForNext() {
    dataStaging.assign(this->pixelCount, 0.0);
    allData.assign(this->pixelCount, 0.0f);
    cloudOrbitals.clear();
    this->orbitalIdx = 0;
    this->allPDVMaximum = 0;
    this->atomZ = 1;
    mStatus.setTo(em::INIT | em::VERT_READY);
}

void CloudManager::resetManager() {
    Manager::resetManager();

    /* for (auto v : pixelColours) {
        delete (v);
    }
    this->pixelColours.clear();
    this->allColours.clear(); */

    this->dataStaging.clear();
    this->idxCulledTolerance.clear();
    this->idxCulledSlider.clear();
    this->norm_constR.clear();
    this->norm_constY.clear();

    this->pixelCount = 0;
    this->colourCount = 0;
    this->colourSize = 0;
    this->orbitalIdx = 0;
    this->allPDVMaximum = 0;
}

/*
 *  Generators
 */

/* void CloudManager::genColourArray() {
    allColours.clear();

    for (int i = 0; i < cloudLayerCount; i++) {
        std::copy(pixelColours[i]->begin(), pixelColours[i]->end(), std::back_inserter(this->allColours));
    }

    this->colourCount = setColourCount();
    this->colourSize = setColourSize();
} */

/*
 *  Getters -- Size
 */

uint CloudManager::getColourSize() {
    return this->colourSize;
}

uint CloudManager::getMaxLayer(double tolerance, int n_max, int divisor) {
    return getMaxRadius(tolerance, n_max) * divisor;
}

uint CloudManager::getMaxRadius(double tolerance, int n_max) {
    int divSciExp = std::abs(floor(log10(tolerance)));
    return cm_maxRadius[divSciExp - 1][n_max - 1];
}

/*
 *  Getters -- Count
 */

/*
 *  Getters -- Data
 */

bool CloudManager::hasVertices() {
    return (this->mStatus.hasAny(em::VERT_READY));
}

bool CloudManager::hasBuffers() {
    return (this->mStatus.hasAny(em::UPD_IBO));
}

/*
 *  Setters -- Size
 */

int CloudManager::setColourSize() {
    int chunks = colourCount ?: setColourCount();
    int chunkSize = sizeof(glm::vec3);

    //std::cout << "allVertices has " << chunks << " chunks of " << chunkSize << " bytes." << std::endl;
    return chunks * chunkSize;
}

/*
 *  Setters -- Count
 */

int CloudManager::setColourCount() {
    return allColours.size();
}

/*
 *  Printers
 */

void CloudManager::printRecipes() {
    for (const auto &key : this->cloudOrbitals) {
        for (const auto &v : key.second) {
            std::cout << std::setw(3) << ++(this->orbitalIdx) << ")  " << key.first << "  " << v.x << " " << v.y << "\n";
        }
    }
    std::cout << std::endl;
}

void CloudManager::printMaxRDP_CSV(const int &n, const int &l, const int &m_l, const double &maxRDP) {
    std::cout << n << "," << l << "," << m_l << "," << maxRDP << "\n";
}

void CloudManager::printBuffer(fvec buf, std::string name) {
    std::ofstream outfile(name);

    if (outfile.is_open()) {
        for (const auto& f : buf) {
            outfile << f;
        }
        outfile << std::endl;
        outfile.close();
    } else {
        std::cout << "Failed to open file." << std::endl;
    }
}

void CloudManager::printBuffer(uvec buf, std::string name) {
    std::ofstream outfile(name);

    if (outfile.is_open()) {
        for (const auto& u : buf) {
            outfile << u;
        }
        outfile << std::endl;
        outfile.close();
    } else {
        std::cout << "Failed to open file." << std::endl;
    }
}

void CloudManager::printTimes() {
    for (auto [lab, t] : std::views::zip(cm_labels, cm_times)) {
        if (t) {
            std::cout << lab << std::setprecision(2) << std::fixed << std::setw(9) << t << " ms\n";
        }
    }
    std::cout << std::endl;
    this->cm_times.fill(0.0);
}

void CloudManager::testThreadingInit() {
    cm_proc_coarse.lock();

    /* With current settings, this will take at least an hour! */

    uint pool_min = 16;
    uint pool_max = std::thread::hardware_concurrency();
    uint pstep = 1;

    uint loop_min = 1;
    uint loop_max = 31;
    uint lstep = 10;

    uint vecs_min = 0;
    uint vecs_max = 6;
    uint vstep = 3;

    uint test_min = 0;
    uint test_max = 8;

    uint vruns = ((vecs_max - vecs_min) / vstep) + 1;
    uint pruns = ((pool_max - pool_min) / pstep) + 1;
    uint lruns = ((loop_max - loop_min) / lstep) + 1;
    uint truns = (test_max - test_min);

    this->cm_pool = pool_max;
    this->cm_loop = 21;
    this->cm_vecs = 2;

    double testtime = createThreaded();
    testtime += (bakeOrbitalsThreadedAlt() * 1.5);
    cullToleranceThreaded();
    cullSliderThreaded();

    // double totaltime = (truns * lruns * pruns * vruns * testtime) + (vruns * pruns * (sleep_time * 1000));
    double totaltime = (truns * lruns * pruns * vruns * testtime);
    std::cout << "Total time expected for test: " << std::setprecision(3) << (totaltime / (1000.0 * 60.0)) << " min" << std::endl;

    std::vector<double> test_times;
    test_times.reserve(vruns * pruns * lruns);
    
    uint progress = 0;
    for (uint v = vecs_min; v <= vecs_max; v += vstep) {
        std::cout << "Progress: " << ++progress << "/" << vruns << "..." << std::endl;
        this->cm_vecs = v;
        for (uint p = pool_min; p <= pool_max; p += pstep) {
            cloudPool.reset(p);
            // sleep(sleep_time);
            for (uint l = loop_min; l <= loop_max; l += lstep) {
                this->cm_loop = l;
                std::vector<double> times(test_max, 0.0);
                for (uint i = test_min; i < test_max; i++) {
                    resetManager();
                    createThreaded();
                    double t = bakeOrbitalsThreadedAlt();
                    times.push_back(t);
                }
                double avg_t = std::accumulate(times.cbegin(), times.cend(), 0.0) / truns;
                test_times.push_back(avg_t);
            }
        }
    }

    for (uint v = 0; v < vruns; v++) {
        for (uint p = 0; p < pruns; p++) {
            std::cout << "" << ((v * vstep) + vecs_min) << "," << ((p * pstep) + pool_min) << ",";
            for (uint l = 0; l < lruns; l++) {
                uint idx = (v * pruns * lruns) + (p * lruns) + l;
                std::cout << test_times[idx] << ",";
            }
            std::cout << "\n";
        }
    }
    std::cout << std::endl;

    cullToleranceThreaded();
    cullSliderThreaded();

    cm_proc_coarse.unlock();
}

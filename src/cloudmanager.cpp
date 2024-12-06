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

// std::execution (via TBB) and Qt both use the emit keyword, so undef for this file to avoid conflicts 
#undef emit
#include <oneapi/dpl/algorithm>
#include <oneapi/dpl/execution>


/**
 * @brief Construct a new CloudManager object.
 *
 * @details
 * This constructor will determine the number of hardware threads available on the system
 * and set the thread pool size for the cloud rendering process to that number.
 *
 * @todo
 * Adjust the thread pool size based on the complexity of the cloud data.
 */
CloudManager::CloudManager() {
    this->cm_pool = std::thread::hardware_concurrency();
    this->cm_loop = 1;
    this->cm_vecs = 0;
}

/**
 * @brief Destruct a CloudManager object.
 *
 * @details
 * This destructor will purge any active tasks in the cloud thread pool and wait for
 * all tasks to finish before clearing the cloud rendering data.
 */
CloudManager::~CloudManager() {
    cloudPool.purge();
    cloudPool.wait();
    resetManager();
}

/**
 * @brief Set the configuration for the cloud rendering process.
 *
 * @details
 * This function is called internally when a new configuration is set.
 *
 * @param config A pointer to the new configuration.
 */
void CloudManager::newConfig(AtomixConfig *config) {
    Manager::newConfig(config);

    this->cloudLayerDivisor = cfg.cloudLayDivisor;
    this->cloudResolution = cfg.cloudResolution;
    this->cloudTolerance = cfg.cloudTolerance;
    this->deg_fac = TWO_PI / this->cloudResolution;
    this->cfg.vert = "gpu_harmonics.vert";
}

/**
 * @brief Receive a new orbital map for the cloud rendering process.
 *
 * @details
 * This function is called by the VKWindow class when a new orbital map is set.
 *
 * @param inMap A pointer to the new orbital map.
 */
void CloudManager::receiveCloudMap(harmap *inMap) {
    this->cloudOrbitals = *inMap;
    this->numOrbitals = countMapRecipes(inMap);
    this->max_n = cloudOrbitals.rbegin()->first;
}

/**
 * @brief Update the cloud rendering manager with a new configuration and orbital map.
 *
 * @details
 * This function is called by the VKWindow class when a new configuration is set.
 * It checks for relevant config or map changes and updates the manager accordingly.
 *
 * @param config A pointer to the new configuration.
 * @param inMap A pointer to the new orbital map.
 * @param generator True if this may generate a new cloud render, False if only culling.
 */
void CloudManager::receiveCloudMapAndConfig(AtomixConfig *config, harmap *inMap, bool generator) {
    cm_proc_coarse.lock();

    if (mStatus.hasNone(em::INIT)) {
        newConfig(config);
        receiveCloudMap(inMap);
        initManager();
        mStatus.set(em::INIT);
        cm_proc_coarse.unlock();
        return;
    }

    // Check for relevant config or map changes -- slider changes can be processed without altering config
    bool widerRadius = false;
    bool newMap = false;
    bool newDivisor = false;
    bool newResolution = false;
    bool newTolerance = false;
    bool newCulling = false;
    bool higherMaxN = false;

    if (generator) {
        widerRadius = (getMaxLayer(config->cloudTolerance, inMap->rbegin()->first, config->cloudLayDivisor) > getMaxLayer(this->cloudTolerance, this->max_n, this->cloudLayerDivisor));
        newMap = cloudOrbitals != (*inMap);
        newDivisor = (this->cloudLayerDivisor != config->cloudLayDivisor);
        newResolution = (this->cloudResolution != config->cloudResolution);
        newTolerance = (this->cloudTolerance != config->cloudTolerance);
        higherMaxN = (mStatus.hasAny(em::VERT_READY)) && (inMap->rbegin()->first > this->max_n);
    }
    newCulling = (this->cfg.CloudCull_x != config->CloudCull_x) || (this->cfg.CloudCull_y != config->CloudCull_y) || (this->cfg.CloudCull_rIn != config->CloudCull_rIn) || (this->cfg.CloudCull_rOut != config->CloudCull_rOut);
    
    bool configChanged = (newDivisor || newResolution || newTolerance);
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
    } else if (newCulling) {
        this->cfg.CloudCull_x = config->CloudCull_x;
        this->cfg.CloudCull_y = config->CloudCull_y;
        this->cfg.CloudCull_rIn = config->CloudCull_rIn;
        this->cfg.CloudCull_rOut = config->CloudCull_rOut;
    }

    // Mark for vecsAndMatrices update if map (orbital recipe) has changed
    if (newMap) {
        this->receiveCloudMap(inMap);
        mStatus.set(em::UPD_MATRICES);
    }

    // Re-gen vertices for new config values if necessary
    if (newVerticesRequired) {
        mStatus.clear(em::VERT_READY);
        cm_times[0] = createThreaded();
    }
    // Re-gen PDVs for new map or if otherwise necessary
    if (newVerticesRequired || newMap) {
        mStatus.clear(em::DATA_READY);
        cm_times[1] = bakeOrbitalsThreaded();
    }
    // Re-cull the indices for tolerance or if otherwise necessary
    if (newVerticesRequired || newMap || newTolerance) {
        mStatus.clear(em::INDEX_GEN);
        cm_times[2] = cullToleranceThreaded();
        if (cfg.cpu) expandPDVsToColours();
    }
    // Re-cull the indices for slider position or if otherwise necessary
    if (newVerticesRequired || newMap || newTolerance || newCulling) {
        mStatus.clear(em::INDEX_READY);
        cm_times[3] = cullSliderThreaded();
    }
    
    if (isProfiling) {
        std::cout << "receiveCloudMapAndConfig() -- Functions took:\n";
        this->printTimes();
    }

    cm_proc_coarse.unlock();
}


/**
 * @brief Initialize the cloud rendering process.
 *
 * @details
 * Called internally to create the first cloud render only. Future updates will
 * be handled by the receiveCloudMapAndConfig function.
 */
void CloudManager::initManager() {
    cm_times[0] = createThreaded();
    cm_times[1] = bakeOrbitalsThreaded();
    cm_times[2] = cullToleranceThreaded();
    if (cfg.cpu) expandPDVsToColours();
    cm_times[3] = cullSliderThreaded();

    if (isProfiling) {
        std::cout << "Init() -- Functions took:\n";
        this->printTimes();
    }

    mStatus.set(em::UPD_MATRICES);
}

// #pragma GCC push_options
// #pragma GCC optimize ("O3")

/**
 * @brief Generates the vertices and colour data for the cloud render (no threading).
 *
 * @details
 * Generates the vertices and colour data for the cloud render. This is the second-most
 * time-consuming part of the cloud rendering process. The function first finds the
 * maximum radius for the given tolerance and then iterates over all the required
 * vertices, creating the positions and initialising the colour data. The
 * wavefunction norms are also calculated here. The function then sets the
 * `em::VERT_READY` status and sets up the vertex array.
 *
 * @return The time taken to complete the function in milliseconds.
 */
double CloudManager::create() {
    assert(mStatus.hasNone(em::VERT_READY));
    steady_clock::time_point begin = steady_clock::now();

    this->max_n = cloudOrbitals.rbegin()->first;
    this->opt_max_radius = getMaxLayer(this->cloudTolerance, this->max_n, this->cloudLayerDivisor);
    int phi_max_local = this->cloudResolution >> 1;
    int theta_max_local = this->cloudResolution;
    double deg_fac_local = this->deg_fac;

    this->pixelCount = opt_max_radius * theta_max_local * phi_max_local;
    vec3 pos = vec3(0.0f);

    // auto beginInner = steady_clock::now();
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
    // auto endInner = steady_clock::now();

    dataStaging.insert(dataStaging.end(), pixelCount, 0.0);
    allData.insert(allData.end(), pixelCount, 0.0f);
    idxCulledTolerance.reserve(pixelCount);

    wavefuncNorms(MAX_SHELLS);

    mStatus.advance(em::INIT, em::VERT_READY);
    genVertexArray();
    steady_clock::time_point end = steady_clock::now();
    // auto createTime = std::chrono::duration<double, std::milli>(endInner - beginInner).count();
    // std::cout << "Create Inner Loop took: " << createTime << " ms" << std::endl;
    return (std::chrono::duration<double, std::milli>(end - begin).count());
}

/**
 * @brief Generate the vertices and colour data for the cloud render in separate threads.
 *
 * @details
 * This function is the same as `create()`, but it is designed to execute in parallel.
 * It is used for generating the initial cloud render when the cloud manager is first
 * initialized.
 *
 * @return The time taken to complete the function in milliseconds.
 */
double CloudManager::createThreaded() {
    assert(mStatus.hasNone(em::VERT_READY));
    cm_proc_fine.lock();
    steady_clock::time_point begin = steady_clock::now();

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
    // auto beginInner = steady_clock::now();
    allVertices.reserve(pixelCount);
    dataStaging.reserve(pixelCount);
    allData.reserve(pixelCount);
    idxCulledTolerance.reserve(pixelCount);

    allVertices.assign(pixelCount, vec3(0.0f));
    dataStaging.assign(pixelCount, 0.0);
    allData.assign(pixelCount, 0.0f);

    wavefuncNorms(MAX_SHELLS);
    // auto endInner = steady_clock::now();
    // auto createTime = std::chrono::duration<double, std::milli>(endInner - beginInner).count();
    // std::cout << "Create Memory Loop took: " << createTime << " ms" << std::endl;

    /*  Compute -- Begin --- This compute portion takes only 6% of create() total time  */
    // auto beginInner = steady_clock::now();
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
    // auto endInner = steady_clock::now();
    // auto createTime = std::chrono::duration<double, std::milli>(endInner - beginInner).count();
    // std::cout << "Create Inner Loop took: " << createTime << " ms" << std::endl;

    /*  Exit  */
    mStatus.set(em::VERT_READY);
    genVertexArray();
    steady_clock::time_point end = steady_clock::now();
    cm_proc_fine.unlock();
    return (std::chrono::duration<double, std::milli>(end - begin).count());
}

/**
 * @brief Populate the dataStaging array with an orbital probability distribution.
 *
 * @details
 * This function is called by bakeOrbitals() to populate the dataStaging array with a
 * probability distribution of the orbital with the given (n, l, m_l) quantum numbers and
 * weight.
 *
 * The function uses the wavefuncRadial() and wavefuncAngExp() functions to compute the
 * radial and angular-exponential parts of the wavefunction. The angular part is then used
 * with the Legendre polynomial using wavefuncAngLeg(). The two parts are then combined into
 * the full angular portion and used to compute the probability density value (PDV) of each
 * point in the dataStaging array.
 *
 * @param n The principal quantum number of the orbital.
 * @param l The orbital angular momentum of the orbital.
 * @param m_l The z-component of the orbital angular momentum of the orbital.
 * @param weight The weight of the orbital in the probability distribution.
 */
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

/**
 * @brief Bakes the orbital recipes to generate the cloud data.
 *
 * @details
 * Iterates through the stored recipes, grouped by N, and calls the
 * genOrbital function for each. The genOrbital function generates the
 * cloud data for each orbital and stores it in the dataStaging vector.
 * The final maximum value of the accumulated vector is stored in
 * allPDVMaximum.
 *
 * @return The time taken to complete the function in milliseconds.
 */
double CloudManager::bakeOrbitals() {
    assert(mStatus.hasNone(em::INIT) && mStatus.hasFirstNotLast(em::VERT_READY, em::DATA_READY));
    steady_clock::time_point begin = steady_clock::now();

    // Iterate through stored recipes, grouped by N
    for (auto const &[key, val] : cloudOrbitals) {
        for (auto const &v : val) {
            genOrbital(key, v.x, v.y, (v.z / static_cast<double>(numOrbitals)));
        }
    }

    // End: check actual max value of accumulated vector
    this->allPDVMaximum = *std::max_element(dataStaging.begin(), dataStaging.end());
    // mStatus.set(em::DATA_GEN);
    steady_clock::time_point end = steady_clock::now();
    return (std::chrono::duration<double, std::milli>(end - begin).count());
}

/**
 * @brief Threading-optimized version of bakeOrbitals, using chunked multithreading with a thread pool.
 * 
 * This version of bakeOrbitals is an attempt at optimizing the cloud generation process by dividing
 * the work into chunks and spawning threads to handle those chunks, from the thread pool.
 * 
 * This function was written to divide the work across orbital recipes first, using temporary staging
 * vectors to store the results of each recipe. Because these vectors can be very large (~GB), they are
 * limited in count and used in a FIFO queue, where as each recipe is completed, higher priority threads
 * are invoked to collapse the temp vector into the final vector, and the temp vector is then reused
 * for another recipe.
 * 
 * Within each recipe, lower priority threads are assigned chunks (a subset of the total radial layers
 * in the model for that recipe), which yield to the higher priority threads when that recipe is completed.
 * 
 * In practice, after extensive profiling, the ideal number of threads to use is always the max/default,
 * the ideal number of temporary vectors to use is always 0, and the ideal chunk size is always 1 layer per thread.
 * 
 * While this function may be updated to use a different work division strategy in future, the current
 * strategy yields a 98% reduction in runtime compared to the single-threaded bakeOrbitals function (6min -> 7sec).
 * 
 * @return The time taken to generate the cloud in milliseconds.
 */
double CloudManager::bakeOrbitalsThreaded() {
    assert(mStatus.hasFirstNotLast(em::VERT_READY, em::DATA_READY));
    cm_proc_fine.lock();
    steady_clock::time_point begin = steady_clock::now();

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
    // steady_clock::time_point inner_begin = steady_clock::now();
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

            // If this is the last recipe, or if there are no temp vectors, use the final vector for all threads
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
                        double R = lagp((n - l - 1), ((l << 1) + 1), rho) * rhol * exp(-rho * 0.5) * radNorm;
                        
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
    steady_clock::time_point end = steady_clock::now();
    cm_proc_fine.unlock();
    return (std::chrono::duration<double, std::milli>(end - begin).count());
}

/**
 * @brief Set all PDVs below the tolerance to zero, and store indices of non-zero PDVs in idxCulledTolerance.
 *
 * @details
 * A PDV has been generated for every vertex in the mesh. This function culls all PDVs
 * below the tolerance and stores the indices of non-zero PDVs in idxCulledTolerance
 * in order to greatly reduce the size of the data buffer and the number of vertices
 * that need to be rendered.
 *
 * @return The time taken to complete the function in milliseconds.
 */
double CloudManager::cullTolerance() {
    // assert(mStatus.hasFirstNotLast(em::DATA_READY, (em::INDEX_GEN | em::INDEX_READY)));
    steady_clock::time_point begin = steady_clock::now();

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
    steady_clock::time_point end = steady_clock::now();
    return (std::chrono::duration<double, std::milli>(end - begin).count());
}

/**
 * @brief Set all PDVs below the tolerance to zero, and store indices of non-zero PDVs in idxCulledTolerance.
 *
 * @details
 * A PDV has been generated for every vertex in the mesh. This function culls all PDVs
 * below the tolerance and stores the indices of non-zero PDVs in idxCulledTolerance
 * in order to greatly reduce the size of the data buffer and the number of vertices
 * that need to be rendered.
 * 
 * This function is a parallelized version of cullTolerance().
 *
 * @return The time taken to complete the function in milliseconds.
 */
double CloudManager::cullToleranceThreaded() {
    assert(mStatus.hasFirstNotLast(em::DATA_READY, em::INDEX_GEN));
    cm_proc_fine.lock();
    steady_clock::time_point begin = steady_clock::now();
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
    steady_clock::time_point end = steady_clock::now();
    cm_proc_fine.unlock();
    return (std::chrono::duration<double, std::milli>(end - begin).count());
}

/**
 * @brief Expand the PDVs to colours, and generate a colour buffer.
 *
 * @details
 * This function takes the PDVs and expands them into a colour representation.
 * The colours are chosen from a palette of 11 different colours, with the
 * PDV value determining the shade of the colour. The colours are then stored
 * in a buffer and a vertex buffer is generated.
 *
 * @return The time taken to complete the function in milliseconds.
 */
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

/**
 * @brief Cull the cloud data based on the slider values.
 *
 * @details
 * This function takes the culling values from the sliders and removes the
 * corresponding vertices from the cloud data. The remaining vertices are then
 * stored in the allIndices vector.
 * 
 * The vertices and indices are generated as:
 * (Layers = [R] * resolution) * [Phi] (pi * resolution) * [Theta] (2pi * resolution).
 * Model may be culled by r, theta, and phi.
 *
 * @return The time taken to complete the function in milliseconds.
 */
double CloudManager::cullSlider() {
    assert(mStatus.hasFirstNotLast(em::INDEX_GEN, em::INDEX_READY));
    steady_clock::time_point begin = steady_clock::now();
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
    steady_clock::time_point end = steady_clock::now();
    return (std::chrono::duration<double, std::milli>(end - begin).count());
}

/**
 * @brief Same as `cullSlider()`, but uses parallel processing to sort and copy indices of non-culled vertices.
 *
 * @details
 * This function is a parallelized version of `cullSlider()`.
 *
 * @return The time taken to complete the function in milliseconds.
 */
double CloudManager::cullSliderThreaded() {
    assert(mStatus.hasFirstNotLast(em::INDEX_GEN, em::INDEX_READY));
    cm_proc_fine.lock();
    steady_clock::time_point begin = steady_clock::now();

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
    steady_clock::time_point end = steady_clock::now();
    cm_proc_fine.unlock();
    return std::chrono::duration<double, std::milli>(end - begin).count();
}

void CloudManager::update([[maybe_unused]] double time) {
    //TODO implement for CPU updates over time
}

    /**
     * @brief cloudTest(int n_max)
     * 
     * Prints the complete list of orbital indices for given n_max to the console.
     * Also checks for and prints any duplicate indices.
     * 
     * @param n_max The maximum principal quantum number to generate indices for.
     */
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

    /**
     * @brief cloudTestCSV
     * 
     * This function is designed to spit out a CSV file that contains the maximum
     * probability density of each orbital for 250 points along the radial axis.
     * The CSV contains the orbital quantum numbers (n, l, ml) for each row,
     * followed by the maximum probability density for each of the 250 radial points.
     * The output is a single, long row with no delimiters between the quantum numbers
     * and the probability densities. The output is not formatted for human readability.
     * 
     * @todo This function should be refactored to emit a header row with the
     *       quantum numbers and the radial points, and to emit a delimiter between
     *       the quantum numbers and the probability densities.
     */
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

    /**
     * @brief Compute the factorial of a given integer `n`.
     *
     * This function takes an integer `n` as input and returns its factorial.
     * The function uses a simple iterative approach to compute the factorial.
     *
     * @param[in] n The integer to compute the factorial of.
     * @returns The factorial of `n`, which is the product of all positive integers less than or equal to `n`.
     */
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

    /**
     * @brief Compute the radial wavefunction for the given quantum numbers.
     *
     * This function takes the principal quantum number `n` and the orbital angular momentum `l`
     * and returns the radial wavefunction evaluated at the given radius `r`.
     * The radial wavefunction is given by the formula:
     * R_{nl}(r) = N_{nl} * L_{n-l-1}^{2l+1}(r/n) * e^{-r/n} * (r/n)^l
     * where L is the associated Laguerre polynomial and N is the normalization constant.
     *
     * @param[in] n The principal quantum number.
     * @param[in] l The orbital angular momentum.
     * @param[in] r The radius at which to evaluate the wavefunction.
     * @returns The radial wavefunction evaluated at `r`.
     */
double CloudManager::wavefuncRadial(int n, int l, double r) {
    double rho = 2.0 * r / static_cast<double>(n);

    double laguerre = lagp((n - l - 1), ((l << 1) + 1), rho);
    double expFunc = exp(-rho/2.0);
    double rhol = pow(rho, l);

    return laguerre * rhol * expFunc * this->norm_constR[DSQ(n, l)];
}

    /**
     * @brief Compute the angular wavefunction for the given quantum numbers.
     *
     * This function takes the orbital angular momentum `l` and the z-component of the
     * orbital angular momentum `m_l` and returns the angular wavefunction evaluated at
     * the given polar angle `theta` and azimuthal angle `phi`. The angular wavefunction
     * is given by the formula:
     * Y_{lm}(\theta, \phi) = exp(i * m_l * \theta) * L_{l}^{m_l}(\cos(\phi)) * N_{lm}
     *
     * @param[in] l The orbital angular momentum.
     * @param[in] m_l The z-component of the orbital angular momentum.
     * @param[in] theta The polar angle at which to evaluate the wavefunction.
     * @param[in] phi The azimuthal angle at which to evaluate the wavefunction.
     * @returns The angular wavefunction evaluated at `(theta, phi)`.
     */
std::complex<double> CloudManager::wavefuncAngular(int l, int m_l, double theta, double phi) {
    using namespace std::complex_literals;
    std::complex<double> ibase = 1i;

    double legendre = legp(l, abs(m_l), cos(phi));
    ibase *= m_l * theta;
    std::complex<double> expFunc = exp(ibase);

    return expFunc * legendre * this->norm_constY[DSQ(l, m_l)];
}

    /**
     * @brief Compute the angular wavefunction exponential term.
     *
     * This function takes the z-component of the orbital angular momentum `m_l` and the
     * polar angle `theta` and returns the exponential term of the angular wavefunction.
     * The exponential term is given by the formula:
     * e^(i*m_l*theta)
     *
     * @param[in] m_l The z-component of the orbital angular momentum.
     * @param[in] theta The polar angle at which to evaluate the wavefunction.
     * @returns The exponential term of the angular wavefunction evaluated at `(m_l, theta)`.
     */
std::complex<double> CloudManager::wavefuncAngExp(int m_l, double theta) {
    using namespace std::complex_literals;
    std::complex<double> ibase = 1i;
    return exp(ibase * (m_l * theta));
}

    /**
     * @brief Compute the associated Legendre polynomial term of the angular wavefunction.
     *
     * This function takes the orbital angular momentum `l` and the z-component of the
     * orbital angular momentum `m_l` and returns the associated Legendre polynomial term
     * of the angular wavefunction evaluated at the given azimuthal angle `phi`.
     *
     * @param[in] l The orbital angular momentum.
     * @param[in] m_l The z-component of the orbital angular momentum.
     * @param[in] phi The azimuthal angle at which to evaluate the wavefunction.
     * @returns The associated Legendre polynomial term of the angular wavefunction evaluated at `(l, m_l, phi)`.
     */
double CloudManager::wavefuncAngLeg(int l, int m_l, double phi) {
    return legp(l, abs(m_l), cos(phi));
}

    /**
     * @brief Compute the probability density value of the orbital wavefunction.
     *
     * This function takes the radial wavefunction term `radial` and the angular
     * wavefunction term `angular` and returns the probability density value of the
     * orbital wavefunction evaluated at the given point.
     *
     * @param[in] radial The radial wavefunction term.
     * @param[in] angular The angular wavefunction term.
     * @returns The probability density value of the orbital wavefunction evaluated at `(radial, angular)`.
     */
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

    /**
     * @brief Compute the probability density value of the orbital wavefunction.
     *
     * This function takes the wavefunction term `Psi` and the radial distance `r`
     * and returns the probability density value of the orbital wavefunction
     * evaluated at the given radial distance.
     *
     * @param[in] Psi The wavefunction term.
     * @param[in] r The radial distance at which to evaluate the wavefunction.
     * @param[in] l The orbital angular momentum.
     * @returns The probability density value of the orbital wavefunction evaluated at `(Psi, r, l)`.
     */
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

    /**
     * @brief Compute the normalizing constants for the orbital wavefunctions.
     *
     * This function takes the maximum principal quantum number `max_n` and
     * computes the normalizing constants for all orbital wavefunctions with
     * n <= max_n. The constants are stored in the `norm_constR` and `norm_constY`
     * maps of the CloudManager object.
     *
     * @param[in] max_n The maximum principal quantum number.
     */
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

    /**
     * @brief Reset the cloud rendering process for the next frame.
     *
     * Clear all data buffers, reset the orbital index, and reset the atom's
     * atomic number.
     */
void CloudManager::clearForNext() {
    dataStaging.assign(this->pixelCount, 0.0);
    allData.assign(this->pixelCount, 0.0f);
    cloudOrbitals.clear();
    this->orbitalIdx = 0;
    this->allPDVMaximum = 0;
    this->atomZ = 1;
    mStatus.setTo(em::INIT | em::VERT_READY);
}

    /**
     * @brief Reset the cloud rendering process to its initial state.
     *
     * Reset the cloud rendering process to its initial state, clearing all
     * data buffers, resetting the orbital index, and resetting the atom's
     * atomic number. This function is generally only called once, when the
     * CloudManager object is created.
     */
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


/*
 *  Getters -- Size
 */

    /**
     * @brief Get the size of the colour data buffer in bytes.
     *
     * @return The size of the colour data buffer in bytes.
     */
uint CloudManager::getColourSize() {
    return this->colourSize;
}

    /**
     * @brief Calculates the maximum layer for a given tolerance, n_max, and divisor.
     *
     * @details
     * The maximum layer is calculated by finding the appropriate row in the
     * cm_maxRadius table and returning the value at n_max - 1, multiplied by
     * the divisor. The row is chosen based on the absolute value of the floor
     * of the base-10 logarithm of the tolerance.
     *
     * @param tolerance The tolerance to calculate the maximum layer for.
     * @param n_max The n_max to calculate the maximum layer for.
     * @param divisor The divisor to multiply the maximum radius by.
     * @return The maximum layer for the given tolerance, n_max, and divisor.
     */
uint CloudManager::getMaxLayer(double tolerance, int n_max, int divisor) {
    uint maxRadius = getMaxRadius(tolerance, n_max);
    return maxRadius * divisor;
}

    /**
     * @brief Calculates the maximum radius for a given tolerance and n_max.
     *
     * @details
     * The maximum radius is calculated by finding the appropriate row in the
     * cm_maxRadius table and returning the value at n_max - 1. The row is
     * chosen based on the absolute value of the floor of the base-10 logarithm
     * of the tolerance.
     *
     * @param tolerance The tolerance to calculate the maximum radius for.
     * @param n_max The n_max to calculate the maximum radius for.
     *
     * @return The maximum radius for the given tolerance and n_max.
     */
uint CloudManager::getMaxRadius(double tolerance, int n_max) {
    uint divSciExp = std::abs(floor(log10(tolerance)));
    uint maxRadius = cm_maxRadius[divSciExp - 1][n_max - 1];
    return maxRadius;
}

/*
 *  Getters -- Count
 */

/*
 *  Getters -- Data
 */

/**
 * @brief Checks if the vertices have been generated.
 *
 * This means that the vertices have been calculated and stored in the member variable allVertices.
 *
 * @return True if the vertices have been created, false otherwise
 */
bool CloudManager::hasVertices() {
    return (this->mStatus.hasAny(em::VERT_READY));
}

/**
 * @brief Checks if the buffers for the CloudManager have been generated.
 *
 * This means that the vertex buffer object, index buffer object, and colour buffer object have been allocated and filled.
 *
 * @return True if the buffers have been created, false otherwise
 */
bool CloudManager::hasBuffers() {
    return (this->mStatus.hasAny(em::UPD_IBO));
}

/*
 *  Setters -- Size
 */

/**
 * @brief Calculate the size of the colour data in bytes.
 *
 * @details
 * The size is calculated as the product of the number of chunks and the size of each chunk.
 * The number of chunks is either the value of `colourCount` if it is not zero, or the result of
 * `setColourCount()`. The size of each chunk is the size of a `glm::vec3`.
 *
 * @return The size of the colour data in bytes.
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

/**
 * @brief Count the total number of recipes in a harmap.
 *
 * @details
 * This function is used to update the CloudManager's count of recipes when the user
 * changes the orbital map. It is called by receiveCloudMapAndConfig() and
 * receiveCloudMap().
 *
 * @param inMap Pointer to the orbital map.
 * @return The total number of recipes in the map.
 */
int CloudManager::countMapRecipes(harmap *inMap) {
    int count = 0;
    for (const auto &key : *inMap) {
        count += key.second.size();
    }
    return count;
}

/*
 *  Printers
 */

/**
 * @brief Prints all orbital recipes to the console, with N and L (ml) values.
 *
 * Prints the values of all orbital recipes, with N, L and ml values. Used for debugging.
 */
void CloudManager::printRecipes() {
    for (const auto &key : this->cloudOrbitals) {
        for (const auto &v : key.second) {
            std::cout << std::setw(3) << ++(this->orbitalIdx) << ")  " << key.first << "  " << v.x << " " << v.y << "\n";
        }
    }
    std::cout << std::endl;
}

/**
 * @brief Prints the maximum radial distance probability value for the given orbital to the console as a CSV line.
 *
 * Prints the values of n, l, ml and the maximum radial distance probability value for the given orbital to the console in a CSV line format.
 * The line is terminated by a newline character.
 *
 * @param n The principal quantum number.
 * @param l The orbital angular momentum quantum number.
 * @param m_l The magnetic quantum number.
 * @param maxRDP The maximum radial distance probability value for the given orbital.
 */
void CloudManager::printMaxRDP_CSV(const int &n, const int &l, const int &m_l, const double &maxRDP) {
    std::cout << n << "," << l << "," << m_l << "," << maxRDP << "\n";
}

/**
 * @brief Prints the contents of the given floating-point vector to the console as a line of text.
 *
 * Prints the contents of the given floating-point vector to the console as a line of text, with each element of the vector separated by a space.
 * The line is terminated by a newline character.
 *
 * @param buf The vector of floating-point numbers to be printed.
 * @param name The name of the file to write to.
 */
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

/**
 * @brief Prints the contents of the given unsigned integer vector to the console as a line of text.
 *
 * Prints the contents of the given unsigned integer vector to the console as a line of text, with each element of the vector separated by a space.
 * The line is terminated by a newline character.
 *
 * @param buf The vector of unsigned integers to be printed.
 * @param name The name of the file to write to.
 */
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

    /**
     * @brief Prints the elapsed time for each stage of the cloud manager processing.
     *
     * Prints the elapsed time for each stage of the cloud manager processing, in milliseconds.
     * The output is in the format "stage: nn.nn ms\n", where "stage" is the name of the stage and
     * "nn.nn" is the elapsed time in milliseconds.
     *
     * @note This function resets the elapsed time counters after printing.
     */
void CloudManager::printTimes() {
    for (auto [lab, t] : std::views::zip(cm_labels, cm_times)) {
        if (t) {
            std::cout << lab << std::setprecision(2) << std::fixed << std::setw(9) << t << " ms\n";
        }
    }
    std::cout << std::endl;
    this->cm_times.fill(0.0);
}

    /**
     * @brief A long-running test function for the threading performance of baking orbitals.
     *
     * This function is a test for the performance of `bakeOrbitalsThreaded()` with different
     * numbers of threads, temporary vectors, and loop iterations. It will take a long time to run
     * and will print out the total time expected for the test.
     *
     * @warning This function is not intended to be called in normal use and is only for testing
     * purposes.
     */
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
    testtime += (bakeOrbitalsThreaded() * 1.5);
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
                    double t = bakeOrbitalsThreaded();
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

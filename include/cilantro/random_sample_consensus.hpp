#pragma once

#include <vector>
#include <algorithm>
#include <random>

namespace cilantro {
    template <class ModelEstimator, class ModelParamsType, class ResidualScalarT>
    class RandomSampleConsensusBase {
    public:
        RandomSampleConsensusBase(size_t sample_size, size_t inlier_count_thresh, size_t max_iter, ResidualScalarT inlier_dist_thresh, bool re_estimate)
                : sample_size_(sample_size),
                  inlier_count_thresh_(inlier_count_thresh),
                  max_iter_(max_iter),
                  inlier_dist_thresh_(inlier_dist_thresh),
                  re_estimate_(re_estimate),
                  iteration_count_(0)
        {}

        inline size_t getSampleSize() const { return sample_size_; }
        inline ModelEstimator& setSampleSize(size_t sample_size) {
            iteration_count_ = 0;
            sample_size_ = sample_size;
            return *static_cast<ModelEstimator*>(this);
        }

        inline size_t getTargetInlierCount() const { return inlier_count_thresh_; }
        inline ModelEstimator& setTargetInlierCount(size_t inlier_count_thres) {
            iteration_count_ = 0;
            inlier_count_thresh_ = inlier_count_thres;
            return *static_cast<ModelEstimator*>(this);
        }

        inline size_t getMaxNumberOfIterations() const { return max_iter_; }
        inline ModelEstimator& setMaxNumberOfIterations(size_t max_iter) {
            iteration_count_ = 0;
            max_iter_ = max_iter;
            return *static_cast<ModelEstimator*>(this);
        }

        inline ResidualScalarT getMaxInlierResidual() const { return inlier_dist_thresh_; }
        inline ModelEstimator& setMaxInlierResidual(ResidualScalarT inlier_dist_thresh) {
            iteration_count_ = 0;
            inlier_dist_thresh_ = inlier_dist_thresh;
            return *static_cast<ModelEstimator*>(this);
        }

        inline bool getReEstimationStep() const { return re_estimate_; }
        inline ModelEstimator& setReEstimationStep(bool re_estimate) {
            iteration_count_ = 0;
            re_estimate_ = re_estimate;
            return *static_cast<ModelEstimator*>(this);
        }

        inline ModelEstimator& getEstimationResults(ModelParamsType &model_params, std::vector<ResidualScalarT> &model_residuals, std::vector<size_t> &model_inliers) {
            if (iteration_count_ == 0) estimate_model_();
            model_params = model_params_;
            model_residuals = model_residuals_;
            model_inliers = model_inliers_;
            return *static_cast<ModelEstimator*>(this);
        }

        inline ModelEstimator& getModelParameters(ModelParamsType &model_params) {
            if (iteration_count_ == 0) estimate_model_();
            model_params = model_params_;
            return *static_cast<ModelEstimator*>(this);
        }

        inline const ModelParamsType& getModelParameters() {
            if (iteration_count_ == 0) estimate_model_();
            return model_params_;
        }

        inline const std::vector<ResidualScalarT>& getModelResiduals() {
            if (iteration_count_ == 0) estimate_model_();
            return model_residuals_;
        }

        inline const std::vector<size_t>& getModelInliers() {
            if (iteration_count_ == 0) estimate_model_();
            return model_inliers_;
        }

        inline bool targetInlierCountAchieved() const { return iteration_count_ > 0 && model_inliers_.size() >= inlier_count_thresh_; }
        inline size_t getPerformedIterationsCount() const { return iteration_count_; }
        inline size_t getNumberOfInliers() const { return model_inliers_.size(); }

    private:
        // Parameters
        size_t sample_size_;
        size_t inlier_count_thresh_;
        size_t max_iter_;
        ResidualScalarT inlier_dist_thresh_;
        bool re_estimate_;

        // Object state and results
        size_t iteration_count_;
        ModelParamsType model_params_;
        std::vector<ResidualScalarT> model_residuals_;
        std::vector<size_t> model_inliers_;

        void estimate_model_() {
            ModelEstimator * estimator = static_cast<ModelEstimator*>(this);
            size_t num_points = estimator->getDataPointsCount();
            if (num_points < sample_size_) sample_size_ = num_points;
            if (inlier_count_thresh_ > num_points) inlier_count_thresh_ = num_points;

            std::random_device rd;
            std::mt19937 rng(rd());

            // Initialize random permutation
            std::vector<size_t> perm(num_points);
            for (size_t i = 0; i < num_points; i++) perm[i] = i;
            std::shuffle(perm.begin(), perm.end(), rng);
            auto sample_start_it = perm.begin();

            iteration_count_ = 0;
            while (iteration_count_ < max_iter_) {
                // Pick a random sample
                if (std::distance(sample_start_it, perm.end()) < sample_size_) {
                    std::shuffle(perm.begin(), perm.end(), rng);
                    sample_start_it = perm.begin();
                }
                std::vector<size_t> sample_ind(sample_start_it, sample_start_it + sample_size_);
                sample_start_it += sample_size_;

                // Fit model to sample and get its inliers
                ModelParamsType model_params_tmp;
                std::vector<size_t> model_inliers_tmp;
                std::vector<ResidualScalarT> model_residuals_tmp;

                estimator->estimateModelParameters(sample_ind, model_params_tmp);
                estimator->computeResiduals(model_params_tmp, model_residuals_tmp);
                model_inliers_tmp.resize(num_points);
                size_t k = 0;
                for (size_t i = 0; i < num_points; i++) {
                    if (model_residuals_tmp[i] <= inlier_dist_thresh_) model_inliers_tmp[k++] = i;
                }
                model_inliers_tmp.resize(k);

                iteration_count_++;
                if (model_inliers_tmp.size() < sample_size_) continue;

                // Update best found
                if (model_inliers_tmp.size() > model_inliers_.size()) {
//                // Re-estimate
//                if (re_estimate_) {
//                    estimator->estimateModelParameters(model_inliers_tmp, model_params_tmp);
//                    estimator->computeResiduals(model_params_tmp, model_residuals_tmp);
//                    model_inliers_tmp.resize(num_points);
//                    k = 0;
//                    for (size_t i = 0; i < num_points; i++){
//                        if (model_residuals_tmp[i] <= inlier_dist_thresh_) model_inliers_tmp[k++] = i;
//                    }
//                    model_inliers_tmp.resize(k);
//                }
                    model_params_ = model_params_tmp;
                    model_residuals_ = std::move(model_residuals_tmp);
                    model_inliers_ = std::move(model_inliers_tmp);
                }

                // Check if target inlier count was reached
                if (model_inliers_.size() >= inlier_count_thresh_) break;
            }

            // Re-estimate
            if (re_estimate_) {
                estimator->estimateModelParameters(model_inliers_, model_params_);
                estimator->computeResiduals(model_params_, model_residuals_);
                model_inliers_.resize(num_points);
                size_t k = 0;
                for (size_t i = 0; i < num_points; i++){
                    if (model_residuals_[i] <= inlier_dist_thresh_) model_inliers_[k++] = i;
                }
                model_inliers_.resize(k);
            }

            //return *estimator;
        }
    };
}


module;

#include <algorithm>
#include <cmath>
#include <exception>
#include <expected>
#include <limits>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <opencv2/core.hpp>
#include <opencv2/dnn.hpp>
#include <opencv2/imgproc.hpp>

module app.detector;

namespace app {

class Detector::Impl {
public:

    auto load(
        const ModelConfig& config,
        float iou_threshold
    ) -> std::expected<void, DetectorError>
    {
        try {
            // Load into a temporary network. A failed reload must not
            // overwrite the currently loaded model.
            auto net = cv::dnn::readNetFromONNX(config.path);

            if (net.empty()) {
                return std::unexpected(DetectorError{
                    "Failed to load the ONNX model: " + config.path
                });
            }

            // Configure the requested execution backend.
            auto backend_result = configure_backend(
                net,
                config.backend);

            if (!backend_result) {
                return std::unexpected(backend_result.error());
            }

            // Commit the configuration only after loading and
            // backend configuration have succeeded.
            net_ = std::move(net);
            input_size_ = config.input_size;
            confidence_threshold_ = config.confidence_threshold;
            iou_threshold_ = iou_threshold;
            classes_ = config.classes;
            backend_ = config.backend;
        }
        catch (const cv::Exception& e) {
            return std::unexpected(DetectorError{
                "OpenCV failed to load or configure the model: " +
                std::string{e.what()}
            });
        }
        catch (const std::exception& e) {
            return std::unexpected(DetectorError{
                "Failed to load the model: " +
                std::string{e.what()}
            });
        }

        return {};
    }

    auto detect(
        const cv::Mat& frame
    ) -> std::expected<std::vector<Detection>, DetectorError>
    {
        if (!net_.has_value()) {
            return std::unexpected(DetectorError{
                "The detector model has not been loaded."
            });
        }

        if (frame.empty()) {
            return std::unexpected(DetectorError{
                "Cannot detect objects in an empty frame."
            });
        }

        if (frame.type() != CV_8UC3) {
            return std::unexpected(DetectorError{
                "The input frame must be a CV_8UC3 BGR image."
            });
        }

        try {
            return detect_impl(frame);
        }
        catch (const cv::Exception& e) {
            return std::unexpected(DetectorError{
                "OpenCV detection error: " +
                std::string{e.what()}
            });
        }
        catch (const std::exception& e) {
            return std::unexpected(DetectorError{
                "Detection failed: " +
                std::string{e.what()}
            });
        }
    }

    [[nodiscard]]
    auto is_loaded() const noexcept -> bool
    {
        return net_.has_value();
    }

private:
    struct PreprocessedInput {
        cv::Mat blob;
        float scale{};
        int pad_x{};
        int pad_y{};
    };

    [[nodiscard]]
    auto configure_backend(
        cv::dnn::Net& net,
        const std::string& requested_backend
    ) const -> std::expected<void, DetectorError>
    {
        if (backend_ == "cpu") {
            net.setPreferableBackend(
                cv::dnn::DNN_BACKEND_OPENCV);

            net.setPreferableTarget(
                cv::dnn::DNN_TARGET_CPU);

            return {};
        }

        if (backend_ == "opencl") {
            net.setPreferableBackend(
                cv::dnn::DNN_BACKEND_OPENCV);

            net.setPreferableTarget(
                cv::dnn::DNN_TARGET_OPENCL);

            return {};
        }

        if (backend_ == "cuda") {
            net.setPreferableBackend(
                cv::dnn::DNN_BACKEND_CUDA);

            net.setPreferableTarget(
                cv::dnn::DNN_TARGET_CUDA);

            return {};
        }

        return std::unexpected(DetectorError{
            "Unsupported backend '" + requested_backend + "'."
        });
    }

    auto detect_impl(
        const cv::Mat& frame
    ) -> std::expected<std::vector<Detection>, DetectorError>
    {
        const auto preprocessed = preprocess(frame);
        const auto output = run_inference(preprocessed.blob);

        // End-to-end output:
        // [1, detections, 6] -> x1, y1, x2, y2, confidence, class_id.
        if (output.size[2] == 6 && output.size[1] <= 300) {
            return decode_end_to_end(
                output,
                preprocessed,
                frame.size());
        }

        auto detections = decode_raw_output(
            output,
            preprocessed,
            frame.size());

        apply_nms(detections);

        return detections;
    }

    auto preprocess(const cv::Mat& frame) const -> PreprocessedInput
    {
        // Preserve aspect ratio and fit the image inside a square.
        const auto scale = std::min(
            static_cast<float>(input_size_) /
                static_cast<float>(frame.cols),
            static_cast<float>(input_size_) /
                static_cast<float>(frame.rows));

        const auto resized_width = static_cast<int>(
            std::round(static_cast<float>(frame.cols) * scale));

        const auto resized_height = static_cast<int>(
            std::round(static_cast<float>(frame.rows) * scale));

        cv::Mat resized;

        cv::resize(
            frame,
            resized,
            cv::Size{resized_width, resized_height});

        const auto pad_x = (input_size_ - resized_width) / 2;
        const auto pad_y = (input_size_ - resized_height) / 2;

        cv::Mat padded(
            input_size_,
            input_size_,
            CV_8UC3,
            cv::Scalar{114, 114, 114});

        resized.copyTo(
            padded(cv::Rect{
                pad_x,
                pad_y,
                resized_width,
                resized_height
            }));

        // Normalize to [0, 1], convert BGR to RGB and create NCHW blob.
        auto blob = cv::dnn::blobFromImage(
            padded,
            1.0 / 255.0,
            cv::Size{input_size_, input_size_},
            cv::Scalar{},
            true,
            false,
            CV_32F);

        return PreprocessedInput{
            .blob = std::move(blob),
            .scale = scale,
            .pad_x = pad_x,
            .pad_y = pad_y
        };
    }

    auto run_inference(const cv::Mat& blob) -> cv::Mat
    {
        net_->setInput(blob);

        std::vector<cv::Mat> outputs;

        net_->forward(
            outputs,
            net_->getUnconnectedOutLayersNames());

        if (outputs.size() != 1) {
            throw std::runtime_error{
                "The model must produce exactly one output tensor."
            };
        }

        auto output = outputs.front();

        if (output.dims != 3 ||
            output.size[0] != 1 ||
            output.size[1] <= 0 ||
            output.size[2] <= 0) {
            throw std::runtime_error{
                "Unexpected model output shape; expected [1, rows, columns]."
            };
        }

        if (output.type() != CV_32F) {
            throw std::runtime_error{
                "The model output tensor must contain 32-bit floats."
            };
        }

        if (!output.isContinuous()) {
            output = output.clone();
        }

        return output;
    }

    auto decode_end_to_end(
        const cv::Mat& output,
        const PreprocessedInput& preprocessed,
        const cv::Size& original_size
    ) const -> std::vector<Detection>
    {
        std::vector<Detection> detections;

        const auto* data = output.ptr<float>();
        const auto rows = output.size[1];
        const auto columns = output.size[2];

        detections.reserve(static_cast<std::size_t>(rows));

        for (int row = 0; row < rows; ++row) {
            const auto* values = data + row * columns;

            const auto confidence = values[4];
            const auto class_value = values[5];

            if (!std::isfinite(confidence) ||
                confidence < confidence_threshold_) {
                continue;
            }

            if (!std::isfinite(class_value) ||
                class_value < 0.0F ||
                class_value >= static_cast<float>(
                    std::numeric_limits<int>::max())) {
                continue;
            }

            const auto class_id = static_cast<int>(class_value);

            // If class names were supplied, ignore invalid class IDs.
            if (!classes_.empty() &&
                static_cast<std::size_t>(class_id) >= classes_.size()) {
                continue;
            }

            const auto x1 =
                (values[0] - static_cast<float>(preprocessed.pad_x)) /
                preprocessed.scale;

            const auto y1 =
                (values[1] - static_cast<float>(preprocessed.pad_y)) /
                preprocessed.scale;

            const auto x2 =
                (values[2] - static_cast<float>(preprocessed.pad_x)) /
                preprocessed.scale;

            const auto y2 =
                (values[3] - static_cast<float>(preprocessed.pad_y)) /
                preprocessed.scale;

            append_detection(
                detections,
                x1, y1, x2, y2,
                confidence,
                class_id,
                original_size);
        }

        // End-to-end models are assumed to perform NMS internally.
        return detections;
    }

    auto decode_raw_output(
        const cv::Mat& output,
        const PreprocessedInput& preprocessed,
        const cv::Size& original_size
    ) const -> std::vector<Detection>
    {
        std::vector<Detection> detections;

        const auto first_dimension = output.size[1];
        const auto second_dimension = output.size[2];

        // Support [1, channels, predictions] and
        // [1, predictions, channels].
        const auto channels_first =
            first_dimension < second_dimension;

        const auto channels = channels_first
            ? first_dimension
            : second_dimension;

        const auto predictions = channels_first
            ? second_dimension
            : first_dimension;

        if (channels < 5) {
            throw std::runtime_error{
                "The raw model output has too few channels."
            };
        }

        const auto* data = output.ptr<float>();

        detections.reserve(
            static_cast<std::size_t>(predictions));

        for (int prediction = 0;
             prediction < predictions;
             ++prediction) {
            const auto value_at = [&](int channel) -> float {
                if (channels_first) {
                    return data[channel * predictions + prediction];
                }

                return data[prediction * channels + channel];
            };

            auto confidence =
                -std::numeric_limits<float>::infinity();

            auto class_id = 0;

            for (int channel = 4; channel < channels; ++channel) {
                const auto score = value_at(channel);

                if (std::isfinite(score) && score > confidence) {
                    confidence = score;
                    class_id = channel - 4;
                }
            }

            if (!std::isfinite(confidence) ||
                confidence < confidence_threshold_) {
                continue;
            }

            if (!classes_.empty() &&
                static_cast<std::size_t>(class_id) >= classes_.size()) {
                continue;
            }

            const auto center_x = value_at(0);
            const auto center_y = value_at(1);
            const auto width = value_at(2);
            const auto height = value_at(3);

            if (!std::isfinite(center_x) ||
                !std::isfinite(center_y) ||
                !std::isfinite(width) ||
                !std::isfinite(height) ||
                width <= 0.0F ||
                height <= 0.0F) {
                continue;
            }

            const auto x1 = center_x - width * 0.5F;
            const auto y1 = center_y - height * 0.5F;
            const auto x2 = center_x + width * 0.5F;
            const auto y2 = center_y + height * 0.5F;

            const auto original_x1 =
                (x1 - static_cast<float>(preprocessed.pad_x)) /
                preprocessed.scale;

            const auto original_y1 =
                (y1 - static_cast<float>(preprocessed.pad_y)) /
                preprocessed.scale;

            const auto original_x2 =
                (x2 - static_cast<float>(preprocessed.pad_x)) /
                preprocessed.scale;

            const auto original_y2 =
                (y2 - static_cast<float>(preprocessed.pad_y)) /
                preprocessed.scale;

            append_detection(
                detections,
                original_x1,
                original_y1,
                original_x2,
                original_y2,
                confidence,
                class_id,
                original_size);
        }

        return detections;
    }

    void append_detection(
        std::vector<Detection>& detections,
        float x1,
        float y1,
        float x2,
        float y2,
        float confidence,
        int class_id,
        const cv::Size& original_size
    ) const
    {
        if (!std::isfinite(x1) ||
            !std::isfinite(y1) ||
            !std::isfinite(x2) ||
            !std::isfinite(y2) ||
            !std::isfinite(confidence) ||
            class_id < 0) {
            return;
        }

        x1 = std::clamp(
            x1, 0.0F, static_cast<float>(original_size.width));

        y1 = std::clamp(
            y1, 0.0F, static_cast<float>(original_size.height));

        x2 = std::clamp(
            x2, 0.0F, static_cast<float>(original_size.width));

        y2 = std::clamp(
            y2, 0.0F, static_cast<float>(original_size.height));

        if (x2 <= x1 || y2 <= y1) {
            return;
        }

        // Preserve fractional coordinates in Rect2f.
        detections.push_back(Detection{
            .box = cv::Rect2f{
                x1,
                y1,
                x2 - x1,
                y2 - y1
            },
            .confidence = confidence,
            .class_id = class_id
        });
    }

    void apply_nms(std::vector<Detection>& detections) const
    {
        // Apply non-maximum suppression independently per class.
        std::unordered_map<
            int,
            std::vector<const Detection*>
        > detections_by_class;

        for (const auto& detection : detections) {
            detections_by_class[detection.class_id].push_back(
                &detection);
        }

        std::vector<Detection> filtered;
        filtered.reserve(detections.size());

        for (const auto& [class_id, class_detections]
             : detections_by_class) {
            (void)class_id;

            std::vector<cv::Rect> boxes;
            std::vector<float> scores;

            boxes.reserve(class_detections.size());
            scores.reserve(class_detections.size());

            for (const auto* detection : class_detections) {
                // NMSBoxes accepts integer rectangles here.
                const auto& box = detection->box;

                boxes.emplace_back(
                    static_cast<int>(std::floor(box.x)),
                    static_cast<int>(std::floor(box.y)),
                    static_cast<int>(std::ceil(box.width)),
                    static_cast<int>(std::ceil(box.height)));

                scores.push_back(detection->confidence);
            }

            std::vector<int> kept_indices;

            cv::dnn::NMSBoxes(
                boxes,
                scores,
                confidence_threshold_,
                iou_threshold_,
                kept_indices);

            for (const auto index : kept_indices) {
                filtered.push_back(
                    *class_detections[
                        static_cast<std::size_t>(index)]);
            }
        }

        detections = std::move(filtered);
    }

    std::optional<cv::dnn::Net> net_;

    int input_size_{640};
    float confidence_threshold_{0.45F};
    float iou_threshold_{0.45F};

    std::vector<std::string> classes_;
    std::string backend_{"cpu"};
};

// Detector public API

Detector::Detector()
    : impl_{std::make_unique<Impl>()}
{
}

Detector::~Detector() = default;

Detector::Detector(Detector&&) noexcept = default;

auto Detector::operator=(Detector&&) noexcept -> Detector& = default;

auto Detector::load(
    const ModelConfig& config,
    float iou_threshold
) -> std::expected<void, DetectorError>
{
    if (!impl_) {
        return std::unexpected(DetectorError{
            "The detector implementation is not available."
        });
    }

    return impl_->load(config, iou_threshold);
}

auto Detector::detect(
    const cv::Mat& frame
) -> std::expected<std::vector<Detection>, DetectorError>
{
    if (!impl_) {
        return std::unexpected(DetectorError{
            "The detector implementation is not available."
        });
    }

    return impl_->detect(frame);
}

auto Detector::is_loaded() const noexcept -> bool
{
    return impl_ && impl_->is_loaded();
}

} // namespace app

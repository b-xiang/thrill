/*******************************************************************************
 * thrill/api/generate_from_file.hpp
 *
 * DIANode for a generate operation. Performs the actual generate operation
 *
 * Part of Project Thrill - http://project-thrill.org
 *
 * Copyright (C) 2015 Alexander Noe <aleexnoe@gmail.com>
 *
 * All rights reserved. Published under the BSD-2 license in the LICENSE file.
 ******************************************************************************/

#pragma once
#ifndef THRILL_API_GENERATE_FROM_FILE_HEADER
#define THRILL_API_GENERATE_FROM_FILE_HEADER

#include <thrill/api/dia.hpp>
#include <thrill/api/source_node.hpp>

#include <fstream>
#include <random>
#include <string>
#include <type_traits>
#include <vector>

namespace thrill {
namespace api {

/*!
 * A DIANode which performs a GenerateFromFile operation. GenerateFromFile uses
 * a file from the file system to generate random inputs. Therefore
 * GenerateFromFile reads the complete file and applies the generator function
 * on each element. Afterwards each worker generates a DIA with a certain number
 * of random (possibly duplicate) elements from the generator file.
 *
 * \tparam ValueType Output type of the Generate operation.
 * \tparam ReadFunction Type of the generate function.
 *
 * \ingroup api_layer
 */
template <typename ValueType, typename GeneratorFunction>
class GenerateFileNode final : public SourceNode<ValueType>
{
public:
    using Super = SourceNode<ValueType>;
    using Super::context_;

    /*!
     * Constructor for a GenerateFileNode. Sets the Context, parents, generator
     * function and file path.
     */
    GenerateFileNode(Context& ctx,
                     GeneratorFunction generator_function,
                     const std::string& path_in,
                     size_t size)
        : Super(ctx, "GenerateFile"),
          generator_function_(generator_function),
          path_in_(path_in),
          size_(size)
    { }

    void PushData(bool /* consume */) final {
        LOG << "GENERATING data to file " << this->id();

        std::ifstream file(path_in_, std::ios::binary);
        assert(file.good());

        std::string line;
        while (std::getline(file, line))
        {
            if (*line.rbegin() == '\r') {
                line.erase(line.length() - 1);
            }
            elements_.push_back(generator_function_(line));
        }

        size_t local_elements;
        size_t elements_per_worker = size_ / context_.num_workers();
        if (context_.num_workers() - 1 == context_.my_rank()) {
            // last worker gets leftovers
            local_elements = size_ -
                             ((context_.num_workers() - 1) * elements_per_worker);
        }
        else {
            local_elements = elements_per_worker;
        }

        std::default_random_engine generator(std::random_device { } ());
        std::uniform_int_distribution<size_t> distribution(0, elements_.size() - 1);

        for (size_t i = 0; i < local_elements; i++) {
            size_t rand_element = distribution(generator);
            this->PushItem(elements_[rand_element]);
        }

        Super::logger_
            << "class" << "GenerateFileNode"
            << "event" << "done";
    }

    void Dispose() final {
        std::vector<ValueType>().swap(elements_);
    }

private:
    //! The read function which is applied on every line read.
    GeneratorFunction generator_function_;
    //! Path of the input file.
    std::string path_in_;
    //! Element vector used for generation
    std::vector<ValueType> elements_;
    //! Size of the output DIA.
    size_t size_;

    static constexpr bool debug = false;
};

/*!
 * GenerateFromFile is a DOp, which reads a file from the file system and
 * applies the generate function on each line. The DIA is generated by
 * pulling random (possibly duplicate) elements out of those generated
 * elements.
 *
 * \tparam GeneratorFunction Type of the generator function.
 *
 * \param ctx Reference to the context object
 * \param filepath Path of the file in the file system
 * \param generator_function Generator function, which is performed on each
 * element
 * \param size Size of the output DIA
 *
 * \ingroup dia_sources
 */
template <typename GeneratorFunction>
auto GenerateFromFile(Context& ctx, const std::string& filepath,
                      const GeneratorFunction& generator_function,
                      size_t size) {

    using GeneratorResult =
              typename common::FunctionTraits<GeneratorFunction>::result_type;

    using GenerateNode =
              GenerateFileNode<GeneratorResult, GeneratorFunction>;

    static_assert(
        std::is_same<
            typename common::FunctionTraits<GeneratorFunction>::template arg<0>,
            const std::string&>::value,
        "GeneratorFunction needs a const std::string& as input");

    auto node = common::MakeCounting<GenerateNode>(
        ctx, generator_function, filepath, size);

    return DIA<GeneratorResult>(node);
}

} // namespace api

//! imported from api namespace
using api::GenerateFromFile;

} // namespace thrill

#endif // !THRILL_API_GENERATE_FROM_FILE_HEADER

/******************************************************************************/

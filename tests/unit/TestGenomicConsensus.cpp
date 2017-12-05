// Copyright (c) 2014-2015, Pacific Biosciences of California, Inc.
//
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted (subject to the limitations in the
// disclaimer below) provided that the following conditions are met:
//
//  * Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//
//  * Redistributions in binary form must reproduce the above
//    copyright notice, this list of conditions and the following
//    disclaimer in the documentation and/or other materials provided
//    with the distribution.
//
//  * Neither the name of Pacific Biosciences nor the names of its
//    contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
// NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
// GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY PACIFIC
// BIOSCIENCES AND ITS CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
// WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
// OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL PACIFIC BIOSCIENCES OR ITS
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
// USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
// OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
// SUCH DAMAGE.

// Author: Derek Barnett

#include <gtest/gtest.h>

#include <algorithm>
#include <string>
#include <vector>

#include <pbbam/BamRecord.h>
#include <pbbam/EntireFileQuery.h>

#include <pacbio/data/Read.h>
#include <pacbio/genomicconsensus/Consensus.h>
#include <pacbio/genomicconsensus/Input.h>
#include <pacbio/genomicconsensus/Intervals.h>
#include <pacbio/genomicconsensus/Output.h>
#include <pacbio/genomicconsensus/Settings.h>
#include <pacbio/genomicconsensus/Sorting.h>

#include <pacbio/genomicconsensus/poa/Poa.h>

#include "TestData.h"

namespace GenomicConsensusTests {

void checkInterval(const PacBio::Data::Interval& query, const size_t expectedLeft,
                   const size_t expectedRight)
{
    EXPECT_EQ(expectedLeft, query.Left());
    EXPECT_EQ(expectedRight, query.Right());
}

}  // namespace GenomicConsensusTests

TEST(GenomicConsensusTests, get_split_intervals_from_bounds)
{
    using PacBio::Data::Interval;
    using PacBio::GenomicConsensus::SplitInterval;
    using GenomicConsensusTests::checkInterval;

    const auto source = Interval{0, 100};
    const auto span = 20;
    const auto intervals = SplitInterval(source, span);
    ASSERT_EQ(5, intervals.size());

    SCOPED_TRACE("get_split_intervals_from_bounds");
    checkInterval(intervals.at(0), 0, 20);
    checkInterval(intervals.at(1), 20, 40);
    checkInterval(intervals.at(2), 40, 60);
    checkInterval(intervals.at(3), 60, 80);
    checkInterval(intervals.at(4), 80, 100);
}

TEST(GenomicConsensusTests, split_intervals_where_last_span_passes_bounds)
{
    using PacBio::Data::Interval;
    using PacBio::GenomicConsensus::SplitInterval;
    using GenomicConsensusTests::checkInterval;

    const auto source = Interval{10, 100};
    const auto span = 20;
    const auto intervals = SplitInterval(source, span);
    ASSERT_EQ(5, intervals.size());

    SCOPED_TRACE("split_intervals_where_last_span_passes_bounds");
    checkInterval(intervals.at(0), 10, 30);
    checkInterval(intervals.at(1), 30, 50);
    checkInterval(intervals.at(2), 50, 70);
    checkInterval(intervals.at(3), 70, 90);
    checkInterval(intervals.at(4), 90, 100);
}

TEST(GenomicConsensusTests, empty_bounds_returns_no_split_intervals)
{
    using PacBio::Data::Interval;
    using PacBio::GenomicConsensus::SplitInterval;

    const auto source = Interval{};
    const auto span = 20;
    const auto intervals = SplitInterval(source, span);
    EXPECT_TRUE(intervals.empty());
}

TEST(GenomicConsensusTests, small_bounds_returns_one_interval)
{
    using PacBio::Data::Interval;
    using PacBio::GenomicConsensus::SplitInterval;
    using GenomicConsensusTests::checkInterval;

    const auto source = Interval{0, 5};
    const auto span = 20;
    const auto intervals = SplitInterval(source, span);
    ASSERT_EQ(1, intervals.size());

    checkInterval(intervals.at(0), 0, 5);
}

TEST(GenomicConsensusTests, intervals_with_overhang)
{
    using PacBio::Data::Interval;
    using PacBio::GenomicConsensus::SplitInterval;
    using GenomicConsensusTests::checkInterval;

    const auto source = Interval{100, 200};
    const auto span = 20;
    const auto overhang = 5;

    const auto intervals = SplitInterval(source, span, overhang);
    ASSERT_EQ(5, intervals.size());

    SCOPED_TRACE("intervals_with_overhang");
    checkInterval(intervals.at(0), 100, 125);
    checkInterval(intervals.at(1), 115, 145);
    checkInterval(intervals.at(2), 135, 165);
    checkInterval(intervals.at(3), 155, 185);
    checkInterval(intervals.at(4), 175, 200);
}

TEST(GenomicConsensusTests, load_reference_windows_from_fasta)
{
    using Input = PacBio::GenomicConsensus::Input;
    using Settings = PacBio::GenomicConsensus::Settings;
    using GenomicConsensusTests::checkInterval;

    Settings settings;
    settings.referenceFilename = tests::DataDir + "/chimera_minimal.fasta";

    const Input input{settings};
    const auto windows = input.ReferenceWindows();
    ASSERT_EQ(28, windows.size());

    SCOPED_TRACE("load_reference_windows_from_fasta");

    // Barcode0--0_Cluster1_Phase1_NumReads297
    checkInterval(windows.at(0).interval, 0, 500);
    checkInterval(windows.at(1).interval, 500, 1000);
    checkInterval(windows.at(2).interval, 1000, 1500);
    checkInterval(windows.at(3).interval, 1500, 2000);
    checkInterval(windows.at(4).interval, 2000, 2500);
    checkInterval(windows.at(5).interval, 2500, 3000);
    checkInterval(windows.at(6).interval, 3000, 3152);

    // Barcode0--0_Cluster1_Phase1_NumReads297
    checkInterval(windows.at(7).interval, 0, 500);
    checkInterval(windows.at(8).interval, 500, 1000);
    checkInterval(windows.at(9).interval, 1000, 1500);
    checkInterval(windows.at(10).interval, 1500, 2000);
    checkInterval(windows.at(11).interval, 2000, 2500);
    checkInterval(windows.at(12).interval, 2500, 3000);
    checkInterval(windows.at(13).interval, 3000, 3137);

    // Barcode0--0_Cluster0_Phase2_NumReads92
    checkInterval(windows.at(14).interval, 0, 500);
    checkInterval(windows.at(15).interval, 500, 1000);
    checkInterval(windows.at(16).interval, 1000, 1500);
    checkInterval(windows.at(17).interval, 1500, 2000);
    checkInterval(windows.at(18).interval, 2000, 2500);
    checkInterval(windows.at(19).interval, 2500, 3000);
    checkInterval(windows.at(20).interval, 3000, 3402);

    // Barcode0--0_Cluster1_Phase3_NumReads56
    checkInterval(windows.at(21).interval, 0, 500);
    checkInterval(windows.at(22).interval, 500, 1000);
    checkInterval(windows.at(23).interval, 1000, 1500);
    checkInterval(windows.at(24).interval, 1500, 2000);
    checkInterval(windows.at(25).interval, 2000, 2500);
    checkInterval(windows.at(26).interval, 2500, 3000);
    checkInterval(windows.at(27).interval, 3000, 3151);
}

TEST(GenomicConsensusTests, enlarged_windows_from_fasta)
{
    using Input = PacBio::GenomicConsensus::Input;
    using ReferenceWindow = PacBio::GenomicConsensus::ReferenceWindow;
    using Settings = PacBio::GenomicConsensus::Settings;
    using GenomicConsensusTests::checkInterval;

    Settings settings;
    settings.referenceFilename = tests::DataDir + "/chimera_minimal.fasta";

    const Input input{settings};
    std::vector<ReferenceWindow> eWindows;
    for (const auto& win : input.ReferenceWindows())
        eWindows.push_back(input.EnlargedWindow(win));
    ASSERT_EQ(28, eWindows.size());

    SCOPED_TRACE("enlarged_windows_from_fasta");

    // Barcode0--0_Cluster1_Phase1_NumReads297
    checkInterval(eWindows.at(0).interval, 0, 505);
    checkInterval(eWindows.at(1).interval, 495, 1005);
    checkInterval(eWindows.at(2).interval, 995, 1505);
    checkInterval(eWindows.at(3).interval, 1495, 2005);
    checkInterval(eWindows.at(4).interval, 1995, 2505);
    checkInterval(eWindows.at(5).interval, 2495, 3005);
    checkInterval(eWindows.at(6).interval, 2995, 3152);

    // Barcode0--0_Cluster1_Phase1_NumReads297
    checkInterval(eWindows.at(7).interval, 0, 505);
    checkInterval(eWindows.at(8).interval, 495, 1005);
    checkInterval(eWindows.at(9).interval, 995, 1505);
    checkInterval(eWindows.at(10).interval, 1495, 2005);
    checkInterval(eWindows.at(11).interval, 1995, 2505);
    checkInterval(eWindows.at(12).interval, 2495, 3005);
    checkInterval(eWindows.at(13).interval, 2995, 3137);

    // Barcode0--0_Cluster0_Phase2_NumReads92
    checkInterval(eWindows.at(14).interval, 0, 505);
    checkInterval(eWindows.at(15).interval, 495, 1005);
    checkInterval(eWindows.at(16).interval, 995, 1505);
    checkInterval(eWindows.at(17).interval, 1495, 2005);
    checkInterval(eWindows.at(18).interval, 1995, 2505);
    checkInterval(eWindows.at(19).interval, 2495, 3005);
    checkInterval(eWindows.at(20).interval, 2995, 3402);

    // Barcode0--0_Cluster1_Phase3_NumReads56
    checkInterval(eWindows.at(21).interval, 0, 505);
    checkInterval(eWindows.at(22).interval, 495, 1005);
    checkInterval(eWindows.at(23).interval, 995, 1505);
    checkInterval(eWindows.at(24).interval, 1495, 2005);
    checkInterval(eWindows.at(25).interval, 1995, 2505);
    checkInterval(eWindows.at(26).interval, 2495, 3005);
    checkInterval(eWindows.at(27).interval, 2995, 3151);
}

TEST(GenomicConsensusTests, no_call_consensus_with_no_call_style)
{
    using Consensus = PacBio::GenomicConsensus::Consensus;
    using NoCallStyle = PacBio::GenomicConsensus::NoCallStyle;
    using ReferenceWindow = PacBio::GenomicConsensus::ReferenceWindow;

    const ReferenceWindow window{"ref1", {0, 8}};
    const std::string refSeq = "ACGTACGT";

    const ReferenceWindow expectedWindow = window;
    const std::string expectedRefSeq = "NNNNNNNN";
    const std::vector<uint8_t> expectedConfidence = {0, 0, 0, 0, 0, 0, 0, 0};

    const auto consensus = Consensus::NoCallConsensus(NoCallStyle::NO_CALL, window, refSeq);
    EXPECT_EQ(expectedWindow, consensus.window);
    EXPECT_EQ(expectedRefSeq, consensus.sequence);
    EXPECT_TRUE(std::equal(consensus.confidence.begin(), consensus.confidence.end(),
                           expectedConfidence.begin(), expectedConfidence.end()));
}

TEST(GenomicConsensusTests, no_call_consensus_with_reference_style)
{
    using Consensus = PacBio::GenomicConsensus::Consensus;
    using NoCallStyle = PacBio::GenomicConsensus::NoCallStyle;
    using ReferenceWindow = PacBio::GenomicConsensus::ReferenceWindow;

    const ReferenceWindow window{"ref1", {0, 8}};
    const std::string refSeq = "ACGTACGT";

    const ReferenceWindow expectedWindow = window;
    const std::string expectedRefSeq = refSeq;
    const std::vector<uint8_t> expectedConfidence = {0, 0, 0, 0, 0, 0, 0, 0};

    const auto consensus = Consensus::NoCallConsensus(NoCallStyle::REFERENCE, window, refSeq);
    EXPECT_EQ(expectedWindow, consensus.window);
    EXPECT_EQ(expectedRefSeq, consensus.sequence);
    EXPECT_TRUE(std::equal(consensus.confidence.begin(), consensus.confidence.end(),
                           expectedConfidence.begin(), expectedConfidence.end()));
}

TEST(GenomicConsensusTests, no_call_consensus_with_lowercase_reference_style)
{
    using Consensus = PacBio::GenomicConsensus::Consensus;
    using NoCallStyle = PacBio::GenomicConsensus::NoCallStyle;
    using ReferenceWindow = PacBio::GenomicConsensus::ReferenceWindow;

    const ReferenceWindow window{"ref1", {0, 8}};
    const std::string refSeq = "ACGTACGT";

    const ReferenceWindow expectedWindow = window;
    const std::string expectedRefSeq = "acgtacgt";
    const std::vector<uint8_t> expectedConfidence = {0, 0, 0, 0, 0, 0, 0, 0};

    const auto consensus =
        Consensus::NoCallConsensus(NoCallStyle::LOWERCASE_REFERENCE, window, refSeq);
    EXPECT_EQ(expectedWindow, consensus.window);
    EXPECT_EQ(expectedRefSeq, consensus.sequence);
    EXPECT_TRUE(std::equal(consensus.confidence.begin(), consensus.confidence.end(),
                           expectedConfidence.begin(), expectedConfidence.end()));
}

class GenomicConsensusSorting : public ::testing::Test
{
protected:
    static void SetUpTestCase()
    {
        window_ = {"All4mer.V2.01_Insert", {0, 500}};
        reads_.reserve(10);

        const std::string inputBamFn =
            tests::DataDir + "/genomicconsensus/all4mer/out.aligned_subreads.bam";
        PacBio::BAM::EntireFileQuery query{inputBamFn};
        for (const auto& read : query) {
            if (reads_.size() == 10) break;
            reads_.emplace_back(read);
        }
    }

    static PacBio::GenomicConsensus::ReferenceWindow window_;
    static std::vector<PacBio::BAM::BamRecord> reads_;
};

PacBio::GenomicConsensus::ReferenceWindow GenomicConsensusSorting::window_;
std::vector<PacBio::BAM::BamRecord> GenomicConsensusSorting::reads_;

TEST_F(GenomicConsensusSorting, sorted_reads_by_longest_and_strand_balanced)
{
    using Sorting = PacBio::GenomicConsensus::Sorting;
    using SortingStrategy = PacBio::GenomicConsensus::SortingStrategy;
    const auto sortedReads =
        Sorting::SortReadsInWindow(reads_, window_, SortingStrategy::LONGEST_AND_STRAND_BALANCED);

    ASSERT_TRUE(sortedReads.size() == 10);
    // 260
    EXPECT_EQ("m141008_060349_42194_c100704972550000001823137703241586_s1_p0/14/2409_2745",
              sortedReads.at(0).FullName());
    EXPECT_EQ("m141008_060349_42194_c100704972550000001823137703241586_s1_p0/14/1669_1990",
              sortedReads.at(1).FullName());
    EXPECT_EQ("m141008_060349_42194_c100704972550000001823137703241586_s1_p0/14/193_534",
              sortedReads.at(2).FullName());
    EXPECT_EQ("m141008_060349_42194_c100704972550000001823137703241586_s1_p0/14/3923_4231",
              sortedReads.at(3).FullName());
    EXPECT_EQ("m141008_060349_42194_c100704972550000001823137703241586_s1_p0/14/9763_10082",
              sortedReads.at(4).FullName());
    // 259
    EXPECT_EQ("m141008_060349_42194_c100704972550000001823137703241586_s1_p0/14/943_1260",
              sortedReads.at(5).FullName());
    EXPECT_EQ("m141008_060349_42194_c100704972550000001823137703241586_s1_p0/14/9022_9354",
              sortedReads.at(6).FullName());
    EXPECT_EQ("m141008_060349_42194_c100704972550000001823137703241586_s1_p0/14/10491_10819",
              sortedReads.at(7).FullName());
    // 258
    EXPECT_EQ("m141008_060349_42194_c100704972550000001823137703241586_s1_p0/14/3189_3513",
              sortedReads.at(8).FullName());
    EXPECT_EQ("m141008_060349_42194_c100704972550000001823137703241586_s1_p0/14/4643_4956",
              sortedReads.at(9).FullName());

    // TODO: check dataset where strand-balance differs from just "longest"
}

TEST_F(GenomicConsensusSorting, sorted_reads_by_longest)
{
    using Sorting = PacBio::GenomicConsensus::Sorting;
    using SortingStrategy = PacBio::GenomicConsensus::SortingStrategy;
    const auto sortedReads = Sorting::SortReadsInWindow(reads_, window_, SortingStrategy::LONGEST);

    ASSERT_TRUE(sortedReads.size() == 10);
    // 260
    EXPECT_EQ("m141008_060349_42194_c100704972550000001823137703241586_s1_p0/14/2409_2745",
              sortedReads.at(0).FullName());
    EXPECT_EQ("m141008_060349_42194_c100704972550000001823137703241586_s1_p0/14/1669_1990",
              sortedReads.at(1).FullName());
    EXPECT_EQ("m141008_060349_42194_c100704972550000001823137703241586_s1_p0/14/193_534",
              sortedReads.at(2).FullName());
    EXPECT_EQ("m141008_060349_42194_c100704972550000001823137703241586_s1_p0/14/3923_4231",
              sortedReads.at(3).FullName());
    EXPECT_EQ("m141008_060349_42194_c100704972550000001823137703241586_s1_p0/14/9763_10082",
              sortedReads.at(4).FullName());
    // 259
    EXPECT_EQ("m141008_060349_42194_c100704972550000001823137703241586_s1_p0/14/943_1260",
              sortedReads.at(5).FullName());
    EXPECT_EQ("m141008_060349_42194_c100704972550000001823137703241586_s1_p0/14/9022_9354",
              sortedReads.at(6).FullName());
    EXPECT_EQ("m141008_060349_42194_c100704972550000001823137703241586_s1_p0/14/10491_10819",
              sortedReads.at(7).FullName());
    // 258
    EXPECT_EQ("m141008_060349_42194_c100704972550000001823137703241586_s1_p0/14/3189_3513",
              sortedReads.at(8).FullName());
    EXPECT_EQ("m141008_060349_42194_c100704972550000001823137703241586_s1_p0/14/4643_4956",
              sortedReads.at(9).FullName());
}

TEST_F(GenomicConsensusSorting, sorted_reads_by_spanning)
{
    using ReferenceWindow = PacBio::GenomicConsensus::ReferenceWindow;
    using Sorting = PacBio::GenomicConsensus::Sorting;
    using SortingStrategy = PacBio::GenomicConsensus::SortingStrategy;

    {  // all should span, so same as file_order
        const ReferenceWindow win{"All4mer.V2.01_Insert", {0, 250}};
        const auto sortedReads = Sorting::SortReadsInWindow(reads_, win, SortingStrategy::SPANNING);

        ASSERT_TRUE(sortedReads.size() == 10);
        EXPECT_EQ("m141008_060349_42194_c100704972550000001823137703241586_s1_p0/14/2409_2745",
                  sortedReads.at(0).FullName());
        EXPECT_EQ("m141008_060349_42194_c100704972550000001823137703241586_s1_p0/14/1669_1990",
                  sortedReads.at(1).FullName());
        EXPECT_EQ("m141008_060349_42194_c100704972550000001823137703241586_s1_p0/14/193_534",
                  sortedReads.at(2).FullName());
        EXPECT_EQ("m141008_060349_42194_c100704972550000001823137703241586_s1_p0/14/3189_3513",
                  sortedReads.at(3).FullName());
        EXPECT_EQ("m141008_060349_42194_c100704972550000001823137703241586_s1_p0/14/3923_4231",
                  sortedReads.at(4).FullName());
        EXPECT_EQ("m141008_060349_42194_c100704972550000001823137703241586_s1_p0/14/4643_4956",
                  sortedReads.at(5).FullName());
        EXPECT_EQ("m141008_060349_42194_c100704972550000001823137703241586_s1_p0/14/943_1260",
                  sortedReads.at(6).FullName());
        EXPECT_EQ("m141008_060349_42194_c100704972550000001823137703241586_s1_p0/14/9022_9354",
                  sortedReads.at(7).FullName());
        EXPECT_EQ("m141008_060349_42194_c100704972550000001823137703241586_s1_p0/14/9763_10082",
                  sortedReads.at(8).FullName());
        EXPECT_EQ("m141008_060349_42194_c100704972550000001823137703241586_s1_p0/14/10491_10819",
                  sortedReads.at(9).FullName());
    }
    {  // none fully span, so empty result
        const auto sortedReads =
            Sorting::SortReadsInWindow(reads_, window_, SortingStrategy::SPANNING);
        EXPECT_TRUE(sortedReads.empty());
    }
}

TEST_F(GenomicConsensusSorting, sorted_reads_by_file_order)
{
    using Sorting = PacBio::GenomicConsensus::Sorting;
    using SortingStrategy = PacBio::GenomicConsensus::SortingStrategy;
    const auto sortedReads =
        Sorting::SortReadsInWindow(reads_, window_, SortingStrategy::FILE_ORDER);

    ASSERT_TRUE(sortedReads.size() == 10);
    EXPECT_EQ("m141008_060349_42194_c100704972550000001823137703241586_s1_p0/14/2409_2745",
              sortedReads.at(0).FullName());
    EXPECT_EQ("m141008_060349_42194_c100704972550000001823137703241586_s1_p0/14/1669_1990",
              sortedReads.at(1).FullName());
    EXPECT_EQ("m141008_060349_42194_c100704972550000001823137703241586_s1_p0/14/193_534",
              sortedReads.at(2).FullName());
    EXPECT_EQ("m141008_060349_42194_c100704972550000001823137703241586_s1_p0/14/3189_3513",
              sortedReads.at(3).FullName());
    EXPECT_EQ("m141008_060349_42194_c100704972550000001823137703241586_s1_p0/14/3923_4231",
              sortedReads.at(4).FullName());
    EXPECT_EQ("m141008_060349_42194_c100704972550000001823137703241586_s1_p0/14/4643_4956",
              sortedReads.at(5).FullName());
    EXPECT_EQ("m141008_060349_42194_c100704972550000001823137703241586_s1_p0/14/943_1260",
              sortedReads.at(6).FullName());
    EXPECT_EQ("m141008_060349_42194_c100704972550000001823137703241586_s1_p0/14/9022_9354",
              sortedReads.at(7).FullName());
    EXPECT_EQ("m141008_060349_42194_c100704972550000001823137703241586_s1_p0/14/9763_10082",
              sortedReads.at(8).FullName());
    EXPECT_EQ("m141008_060349_42194_c100704972550000001823137703241586_s1_p0/14/10491_10819",
              sortedReads.at(9).FullName());
}

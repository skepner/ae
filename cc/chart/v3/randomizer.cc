#include "utils/string.hh"
#include "chart/v3/randomizer.hh"
#include "chart/v3/chart.hh"
#include "chart/v3/optimize.hh"

// ----------------------------------------------------------------------

std::shared_ptr<ae::chart::v3::LayoutRandomizerPlain> ae::chart::v3::randomizer_plain_with_table_max_distance(const Chart& chart, const Projection& projection, LayoutRandomizer::seed_t seed)
{
    auto cb = projection.forced_column_bases();
    if (cb.empty())
        cb = chart.column_bases(projection.minimum_column_basis());
    const auto max_distance = chart.titers().max_distance(cb);
    return std::make_shared<LayoutRandomizerPlain>(max_distance, seed);

} // ae::chart::v3::randomizer_plain_with_table_max_distance

// ----------------------------------------------------------------------

std::shared_ptr<ae::chart::v3::LayoutRandomizer> ae::chart::v3::randomizer_plain_with_current_layout_area(const Chart& /*chart*/, const Projection& projection, double diameter_multiplier, LayoutRandomizer::seed_t seed)
{
    const auto mm = projection.layout().minmax();
    auto sq = [](double v) { return v*v; };
    const auto diameter = std::sqrt(std::accumulate(mm.begin(), mm.end(), 0.0, [&sq](double sum, const auto& p) { return sum + sq(p.second - p.first); }));
    return std::make_shared<LayoutRandomizerPlain>(diameter * diameter_multiplier, seed);

} // ae::chart::v3::randomizer_plain_with_current_layout_area

// ----------------------------------------------------------------------

std::shared_ptr<ae::chart::v3::LayoutRandomizer> ae::chart::v3::randomizer_border_with_current_layout_area(const Chart& /*chart*/, const Projection& projection, double diameter_multiplier, const ae::draw::v2::LineSide& line_side, LayoutRandomizer::seed_t seed)
{
    const auto mm = projection.layout().minmax();
    auto sq = [](double v) { return v*v; };
    const auto diameter = std::sqrt(std::accumulate(mm.begin(), mm.end(), 0.0, [&sq](double sum, const auto& p) { return sum + sq(p.second - p.first); }));
    return std::make_shared<LayoutRandomizerWithLineBorder>(diameter * diameter_multiplier, line_side, seed);

} // ae::chart::v3::randomizer_border_with_current_layout_area

// ----------------------------------------------------------------------

inline std::shared_ptr<ae::chart::v3::LayoutRandomizer> randomizer_plain_from_sample_optimization_internal(const ae::chart::v3::Chart& chart, ae::chart::v3::Projection&& projection, const ae::chart::v3::Stress& stress,
                                                                                                    double diameter_multiplier, ae::chart::v3::LayoutRandomizer::seed_t seed)
{
    auto rnd = randomizer_plain_with_table_max_distance(chart, projection, seed);
    projection.randomize_layout(rnd);
    ae::chart::v3::optimize(ae::chart::v3::optimization_method::alglib_cg_pca, stress, projection.layout().data(), projection.layout().data_size(),
                            ae::chart::v3::optimization_precision::very_rough);
    auto sq = [](double v) { return v * v; };
    const auto mm = projection.layout().minmax();
    const auto diameter = std::sqrt(std::accumulate(mm.begin(), mm.end(), 0.0, [&sq](double sum, const auto& p) { return sum + sq(p.second - p.first); }));
    if (std::isnan(diameter) || float_zero(diameter))
        throw std::runtime_error{AD_FORMAT("randomizer_plain_from_sample_optimization_internal: diameter is {}", diameter)};
    rnd->diameter(diameter * diameter_multiplier);
    return rnd;

} // randomizer_plain_from_sample_optimization_internal

// ----------------------------------------------------------------------

std::shared_ptr<ae::chart::v3::LayoutRandomizer> ae::chart::v3::randomizer_plain_from_sample_optimization(const Chart& chart, const Projection& projection, const Stress& stress, double diameter_multiplier, LayoutRandomizer::seed_t seed)
{
    return randomizer_plain_from_sample_optimization_internal(chart, Projection(projection.number_of_points(), projection.number_of_dimensions(), projection.minimum_column_basis()), stress, diameter_multiplier, seed);

} // ae::chart::v3::randomizer_plain_from_sample_optimization

// ----------------------------------------------------------------------

std::shared_ptr<ae::chart::v3::LayoutRandomizer> ae::chart::v3::randomizer_plain_from_sample_optimization(const Chart& chart, const Stress& stress, number_of_dimensions_t number_of_dimensions, minimum_column_basis mcb, double diameter_multiplier, LayoutRandomizer::seed_t seed)
{
    return randomizer_plain_from_sample_optimization_internal(chart, Projection(chart.number_of_points(), number_of_dimensions, mcb), stress, diameter_multiplier, seed);

} // ae::chart::v3::randomizer_plain_from_sample_optimization

// ----------------------------------------------------------------------

/*
   A C-program for MT19937, with initialization improved 2002/1/26.
   Coded by Takuji Nishimura and Makoto Matsumoto.

   Before using, initialize the state by using init_genrand(seed)
   or init_by_array(init_key, key_length).

   Copyright (C) 1997 - 2002, Makoto Matsumoto and Takuji Nishimura,
   All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

     1. Redistributions of source code must retain the above copyright
        notice, this list of conditions and the following disclaimer.

     2. Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.

     3. The names of its contributors may not be used to endorse or promote
        products derived from this software without specific prior written
        permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


   Any feedback is very welcome.
   http://www.math.sci.hiroshima-u.ac.jp/~m-mat/MT/emt.html
   email: m-mat @ math.sci.hiroshima-u.ac.jp (remove space)
*/

// C-program for MT19937 is adopted for C++

void ae::chart::v3::mt19937_2002::init(unsigned long seed)
{
    mt_[0]= seed & 0xffffffffUL;
    for (size_t mti = 1; mti < N; ++mti) {
        mt_[mti] = (1812433253UL * (mt_[mti-1] ^ (mt_[mti-1] >> 30)) + mti);
        /* See Knuth TAOCP Vol2. 3rd Ed. P.106 for multiplier. */
        /* In the previous versions, MSBs of the seed affect   */
        /* only MSBs of the array mt_[].                        */
        /* 2002/01/09 modified by Makoto Matsumoto             */
        mt_[mti] &= 0xffffffffUL;
        /* for >32 bit machines */
    }
    mti_ = N;
}

// generates a random number on [0,0xffffffff]-interval
unsigned long ae::chart::v3::mt19937_2002::operator()()
{
    constexpr const auto M{397ul};
    constexpr const auto MATRIX_A{0x9908b0dfUL};   // constant vector a
    constexpr const auto UPPER_MASK{0x80000000UL}; // most significant w-r bits
    constexpr const auto LOWER_MASK{0x7fffffffUL}; // least significant r bits

    // unsigned long y;
    static unsigned long mag01[2]={0x0UL, MATRIX_A};
    /* mag01[x] = x * MATRIX_A  for x=0,1 */

    if (mti_ >= N) { /* generate N words at one time */
        size_t kk{0};
        for (; kk < (N - M); ++kk) {
            const auto y = (mt_[kk] & UPPER_MASK) | (mt_[kk + 1] & LOWER_MASK);
            mt_[kk] = mt_[kk + M] ^ (y >> 1) ^ mag01[y & 0x1UL];
        }
        for (; kk < (N - 1); ++kk) {
            const auto y = (mt_[kk] & UPPER_MASK) | (mt_[kk + 1] & LOWER_MASK);
            mt_[kk] = mt_[kk + (M - N)] ^ (y >> 1) ^ mag01[y & 0x1ul];
        }
        const auto y = (mt_[N - 1] & UPPER_MASK) | (mt_[0] & LOWER_MASK);
        mt_[N - 1] = mt_[M - 1] ^ (y >> 1) ^ mag01[y & 0x1UL];

        mti_ = 0;
    }

    auto y = mt_[mti_++];

    /* Tempering */
    y ^= (y >> 11);
    y ^= (y << 7) & 0x9d2c5680UL;
    y ^= (y << 15) & 0xefc60000UL;
    y ^= (y >> 18);

    return y;
}

// ----------------------------------------------------------------------

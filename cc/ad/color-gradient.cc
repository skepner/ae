#include <cmath>

#include "ext/range-v3.hh"
#include "ad/enumerate.hh"
#include "ad/color-gradient.hh"

namespace acmacs::color
{
    using RGB = std::array<uint32_t, 3>;
    // constexpr inline RGB rgb(Color src) { return RGB{src.redI(), src.greenI(), src.blueI()}; }
    constexpr inline Color from(const RGB& rgb) { return Color{(rgb[0] << 16) | (rgb[1] << 8) | rgb[2]}; }
}

// ----------------------------------------------------------------------

// Copyright (c) 2015, Politiken Journalism <emil.bay@pol.dk>

// Permission to use, copy, modify, and/or distribute this software for any
// purpose with or without fee is hereby granted, provided that the above
// copyright notice and this permission notice appear in all copies.

// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
// WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
// ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
// WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
// ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
// OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

// https://github.com/politiken-journalism/scale-color-perceptual/blob/master/hex/viridis.json

static std::array sScaleColorPerceptualViridis{
    0x440154, 0x440256, 0x450457, 0x450559, 0x46075a, 0x46085c, 0x460a5d, 0x460b5e, 0x470d60, 0x470e61, 0x471063, 0x471164, 0x471365, 0x481467, 0x481668, 0x481769,
    0x48186a, 0x481a6c, 0x481b6d, 0x481c6e, 0x481d6f, 0x481f70, 0x482071, 0x482173, 0x482374, 0x482475, 0x482576, 0x482677, 0x482878, 0x482979, 0x472a7a, 0x472c7a,
    0x472d7b, 0x472e7c, 0x472f7d, 0x46307e, 0x46327e, 0x46337f, 0x463480, 0x453581, 0x453781, 0x453882, 0x443983, 0x443a83, 0x443b84, 0x433d84, 0x433e85, 0x423f85,
    0x424086, 0x424186, 0x414287, 0x414487, 0x404588, 0x404688, 0x3f4788, 0x3f4889, 0x3e4989, 0x3e4a89, 0x3e4c8a, 0x3d4d8a, 0x3d4e8a, 0x3c4f8a, 0x3c508b, 0x3b518b,
    0x3b528b, 0x3a538b, 0x3a548c, 0x39558c, 0x39568c, 0x38588c, 0x38598c, 0x375a8c, 0x375b8d, 0x365c8d, 0x365d8d, 0x355e8d, 0x355f8d, 0x34608d, 0x34618d, 0x33628d,
    0x33638d, 0x32648e, 0x32658e, 0x31668e, 0x31678e, 0x31688e, 0x30698e, 0x306a8e, 0x2f6b8e, 0x2f6c8e, 0x2e6d8e, 0x2e6e8e, 0x2e6f8e, 0x2d708e, 0x2d718e, 0x2c718e,
    0x2c728e, 0x2c738e, 0x2b748e, 0x2b758e, 0x2a768e, 0x2a778e, 0x2a788e, 0x29798e, 0x297a8e, 0x297b8e, 0x287c8e, 0x287d8e, 0x277e8e, 0x277f8e, 0x27808e, 0x26818e,
    0x26828e, 0x26828e, 0x25838e, 0x25848e, 0x25858e, 0x24868e, 0x24878e, 0x23888e, 0x23898e, 0x238a8d, 0x228b8d, 0x228c8d, 0x228d8d, 0x218e8d, 0x218f8d, 0x21908d,
    0x21918c, 0x20928c, 0x20928c, 0x20938c, 0x1f948c, 0x1f958b, 0x1f968b, 0x1f978b, 0x1f988b, 0x1f998a, 0x1f9a8a, 0x1e9b8a, 0x1e9c89, 0x1e9d89, 0x1f9e89, 0x1f9f88,
    0x1fa088, 0x1fa188, 0x1fa187, 0x1fa287, 0x20a386, 0x20a486, 0x21a585, 0x21a685, 0x22a785, 0x22a884, 0x23a983, 0x24aa83, 0x25ab82, 0x25ac82, 0x26ad81, 0x27ad81,
    0x28ae80, 0x29af7f, 0x2ab07f, 0x2cb17e, 0x2db27d, 0x2eb37c, 0x2fb47c, 0x31b57b, 0x32b67a, 0x34b679, 0x35b779, 0x37b878, 0x38b977, 0x3aba76, 0x3bbb75, 0x3dbc74,
    0x3fbc73, 0x40bd72, 0x42be71, 0x44bf70, 0x46c06f, 0x48c16e, 0x4ac16d, 0x4cc26c, 0x4ec36b, 0x50c46a, 0x52c569, 0x54c568, 0x56c667, 0x58c765, 0x5ac864, 0x5cc863,
    0x5ec962, 0x60ca60, 0x63cb5f, 0x65cb5e, 0x67cc5c, 0x69cd5b, 0x6ccd5a, 0x6ece58, 0x70cf57, 0x73d056, 0x75d054, 0x77d153, 0x7ad151, 0x7cd250, 0x7fd34e, 0x81d34d,
    0x84d44b, 0x86d549, 0x89d548, 0x8bd646, 0x8ed645, 0x90d743, 0x93d741, 0x95d840, 0x98d83e, 0x9bd93c, 0x9dd93b, 0xa0da39, 0xa2da37, 0xa5db36, 0xa8db34, 0xaadc32,
    0xaddc30, 0xb0dd2f, 0xb2dd2d, 0xb5de2b, 0xb8de29, 0xbade28, 0xbddf26, 0xc0df25, 0xc2df23, 0xc5e021, 0xc8e020, 0xcae11f, 0xcde11d, 0xd0e11c, 0xd2e21b, 0xd5e21a,
    0xd8e219, 0xdae319, 0xdde318, 0xdfe318, 0xe2e418, 0xe5e419, 0xe7e419, 0xeae51a, 0xece51b, 0xefe51c, 0xf1e51d, 0xf4e61e, 0xf6e620, 0xf8e621, 0xfbe723, 0xfde725
};

// ----------------------------------------------------------------------

Color acmacs::color::perceptually_uniform_heatmap(size_t total_colors, size_t color_index)
{
    if (color_index >= total_colors)
        color_index = total_colors - 1;
    const auto step = static_cast<double>(sScaleColorPerceptualViridis.size()) / static_cast<double>(total_colors - 1);
    const auto offset = static_cast<size_t>(std::lround(static_cast<double>(color_index) * step));
    if (offset >= sScaleColorPerceptualViridis.size())
        return sScaleColorPerceptualViridis.back();
    else
        return sScaleColorPerceptualViridis[offset];

} // acmacs::color::perceptually_uniform_heatmap

// ----------------------------------------------------------------------
// https://bsou.io/posts/color-gradients-with-python
// ----------------------------------------------------------------------

constexpr double factorial[] = {1.0, 1.0, 2.0}; // , 6.0};

inline double bernstein(double t_arg, size_t n_arg, size_t i_arg)
{
    return factorial[n_arg] / (factorial[i_arg] * factorial[n_arg - i_arg]) * std::pow(1.0 - t_arg, n_arg - i_arg) * std::pow(t_arg, i_arg);
}

template <typename Colors> static Color bezier_interpolation(const Colors& colors, double t_arg)
{
    acmacs::color::RGB result{0, 0, 0};
    for (auto [index, color] : acmacs::enumerate(colors)) {
        const auto bernst = bernstein(t_arg, colors.size() - 1, index);
        result[0] += static_cast<uint32_t>(color.redI()   * bernst);
        result[1] += static_cast<uint32_t>(color.greenI() * bernst);
        result[2] += static_cast<uint32_t>(color.blueI()  * bernst);
    }
    // fmt::print(stderr, ">>>> bezier_interpolation {}\n", result);
    return acmacs::color::from(result);
}

// ----------------------------------------------------------------------

std::vector<Color> acmacs::color::bezier_gradient(Color c1, Color c2, Color c3, size_t output_size)
{
    const std::array colors{c1, c2, c3};
    return ranges::views::ints(0UL, output_size)
            | ranges::views::transform([&colors,output_size](auto index) { return bezier_interpolation(colors, static_cast<double>(index) / static_cast<double>(output_size - 1)); })
            | ranges::to<std::vector>;

} // acmacs::color::bezier_gradient

// ----------------------------------------------------------------------

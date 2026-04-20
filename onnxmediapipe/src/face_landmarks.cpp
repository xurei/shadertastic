/******************************************************************************
    Copyright (C) 2023 by xurei <xureilab@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

// NOTE : this file has been taken from https://github.com/intel/openvino-plugins-for-obs-studio and modified to use ONNX instead

#include <vector>
#include <algorithm>
#include <iostream>
#include <opencv2/core.hpp>
#include <obs-module.h>
#include "onnxmediapipe/face_landmarks.h"
#include "onnxmediapipe/landmark_refinement_indices.h"

#include "../../src/logging_functions.hpp"
#include "../../src/fdebug.h"
#include "../../src/util/time_util.hpp"

FILE* faceLandmarksDebugFile;

namespace onnxmediapipe
{
    const Ort::RunOptions runOptions{nullptr};

#define FACE_TRIANGLES_COUNT 898
static const cv::Vec3i face_triangles[FACE_TRIANGLES_COUNT] = {
    cv::Vec3i(0, 11, 302),
    cv::Vec3i(0, 37, 72),
    cv::Vec3i(0, 72, 11),
    cv::Vec3i(0, 164, 37),
    cv::Vec3i(0, 267, 164),
    cv::Vec3i(0, 302, 267),
    cv::Vec3i(1, 4, 45),
    cv::Vec3i(1, 19, 274),
    cv::Vec3i(1, 44, 19),
    cv::Vec3i(1, 45, 44),
    cv::Vec3i(1, 274, 275),
    cv::Vec3i(1, 275, 4),
    cv::Vec3i(2, 94, 141),
    cv::Vec3i(2, 97, 164),
    cv::Vec3i(2, 141, 97),
    cv::Vec3i(2, 164, 326),
    cv::Vec3i(2, 326, 370),
    cv::Vec3i(2, 370, 94),
    cv::Vec3i(3, 51, 195),
    cv::Vec3i(3, 195, 197),
    cv::Vec3i(3, 196, 236),
    cv::Vec3i(3, 197, 196),
    cv::Vec3i(3, 236, 51),
    cv::Vec3i(4, 5, 45),
    cv::Vec3i(4, 275, 5),
    cv::Vec3i(5, 51, 45),
    cv::Vec3i(5, 195, 51),
    cv::Vec3i(5, 275, 281),
    cv::Vec3i(5, 281, 195),
    cv::Vec3i(6, 122, 196),
    cv::Vec3i(6, 168, 122),
    cv::Vec3i(6, 196, 197),
    cv::Vec3i(6, 197, 419),
    cv::Vec3i(6, 351, 168),
    cv::Vec3i(6, 419, 351),
    cv::Vec3i(7, 25, 110),
    cv::Vec3i(7, 33, 25),
    cv::Vec3i(7, 110, 163),
    cv::Vec3i(7, 163, 246),
    cv::Vec3i(7, 246, 33),
    cv::Vec3i(8, 9, 55),
    cv::Vec3i(8, 55, 193),
    cv::Vec3i(8, 168, 417),
    cv::Vec3i(8, 193, 168),
    cv::Vec3i(8, 285, 9),
    cv::Vec3i(8, 417, 285),
    cv::Vec3i(9, 107, 55),
    cv::Vec3i(9, 151, 107),
    cv::Vec3i(9, 285, 336),
    cv::Vec3i(9, 336, 151),
    cv::Vec3i(10, 108, 151),
    cv::Vec3i(10, 109, 108),
    cv::Vec3i(10, 151, 337),
    cv::Vec3i(10, 337, 338),
    cv::Vec3i(11, 12, 302),
    cv::Vec3i(11, 72, 12),
    cv::Vec3i(12, 13, 268),
    cv::Vec3i(12, 38, 13),
    cv::Vec3i(12, 72, 38),
    cv::Vec3i(12, 268, 302),
    cv::Vec3i(13, 14, 317),
    cv::Vec3i(13, 38, 82),
    cv::Vec3i(13, 82, 87),
    cv::Vec3i(13, 87, 14),
    cv::Vec3i(13, 312, 268),
    cv::Vec3i(13, 317, 312),
    cv::Vec3i(14, 15, 316),
    cv::Vec3i(14, 86, 15),
    cv::Vec3i(14, 87, 86),
    cv::Vec3i(14, 316, 317),
    cv::Vec3i(15, 16, 315),
    cv::Vec3i(15, 85, 16),
    cv::Vec3i(15, 86, 85),
    cv::Vec3i(15, 315, 316),
    cv::Vec3i(16, 17, 314),
    cv::Vec3i(16, 84, 17),
    cv::Vec3i(16, 85, 84),
    cv::Vec3i(16, 314, 315),
    cv::Vec3i(17, 18, 314),
    cv::Vec3i(17, 84, 18),
    cv::Vec3i(18, 83, 200),
    cv::Vec3i(18, 84, 83),
    cv::Vec3i(18, 200, 313),
    cv::Vec3i(18, 313, 314),
    cv::Vec3i(19, 44, 125),
    cv::Vec3i(19, 94, 354),
    cv::Vec3i(19, 125, 94),
    cv::Vec3i(19, 354, 274),
    cv::Vec3i(20, 60, 99),
    cv::Vec3i(20, 79, 60),
    cv::Vec3i(20, 99, 242),
    cv::Vec3i(20, 238, 79),
    cv::Vec3i(20, 242, 238),
    cv::Vec3i(21, 71, 54),
    cv::Vec3i(21, 139, 71),
    cv::Vec3i(21, 162, 139),
    cv::Vec3i(22, 23, 231),
    cv::Vec3i(22, 26, 154),
    cv::Vec3i(22, 153, 23),
    cv::Vec3i(22, 154, 153),
    cv::Vec3i(22, 231, 232),
    cv::Vec3i(22, 232, 26),
    cv::Vec3i(23, 24, 230),
    cv::Vec3i(23, 144, 24),
    cv::Vec3i(23, 145, 144),
    cv::Vec3i(23, 153, 145),
    cv::Vec3i(23, 230, 231),
    cv::Vec3i(24, 110, 229),
    cv::Vec3i(24, 144, 110),
    cv::Vec3i(24, 229, 230),
    cv::Vec3i(25, 31, 228),
    cv::Vec3i(25, 33, 130),
    cv::Vec3i(25, 130, 226),
    cv::Vec3i(25, 226, 31),
    cv::Vec3i(25, 228, 110),
    cv::Vec3i(26, 112, 155),
    cv::Vec3i(26, 155, 154),
    cv::Vec3i(26, 232, 112),
    cv::Vec3i(27, 28, 223),
    cv::Vec3i(27, 29, 159),
    cv::Vec3i(27, 159, 28),
    cv::Vec3i(27, 223, 29),
    cv::Vec3i(28, 56, 222),
    cv::Vec3i(28, 158, 56),
    cv::Vec3i(28, 159, 158),
    cv::Vec3i(28, 222, 223),
    cv::Vec3i(29, 30, 160),
    cv::Vec3i(29, 160, 159),
    cv::Vec3i(29, 223, 224),
    cv::Vec3i(29, 224, 30),
    cv::Vec3i(30, 161, 160),
    cv::Vec3i(30, 224, 225),
    cv::Vec3i(30, 225, 247),
    cv::Vec3i(30, 247, 161),
    cv::Vec3i(31, 35, 111),
    cv::Vec3i(31, 111, 117),
    cv::Vec3i(31, 117, 228),
    cv::Vec3i(31, 226, 35),
    cv::Vec3i(32, 140, 208),
    cv::Vec3i(32, 170, 140),
    cv::Vec3i(32, 194, 211),
    cv::Vec3i(32, 201, 194),
    cv::Vec3i(32, 208, 201),
    cv::Vec3i(32, 211, 170),
    cv::Vec3i(33, 246, 247),
    cv::Vec3i(33, 247, 130),
    cv::Vec3i(34, 116, 143),
    cv::Vec3i(34, 127, 227),
    cv::Vec3i(34, 139, 162),
    cv::Vec3i(34, 143, 139),
    cv::Vec3i(34, 162, 127),
    cv::Vec3i(34, 227, 116),
    cv::Vec3i(35, 124, 156),
    cv::Vec3i(35, 143, 111),
    cv::Vec3i(35, 156, 143),
    cv::Vec3i(35, 226, 124),
    cv::Vec3i(36, 100, 101),
    cv::Vec3i(36, 101, 205),
    cv::Vec3i(36, 129, 142),
    cv::Vec3i(36, 142, 100),
    cv::Vec3i(36, 203, 129),
    cv::Vec3i(36, 205, 206),
    cv::Vec3i(36, 206, 203),
    cv::Vec3i(37, 39, 72),
    cv::Vec3i(37, 164, 167),
    cv::Vec3i(37, 167, 39),
    cv::Vec3i(38, 41, 82),
    cv::Vec3i(38, 72, 73),
    cv::Vec3i(38, 73, 41),
    cv::Vec3i(39, 40, 73),
    cv::Vec3i(39, 73, 72),
    cv::Vec3i(39, 92, 40),
    cv::Vec3i(39, 165, 92),
    cv::Vec3i(39, 167, 165),
    cv::Vec3i(40, 74, 73),
    cv::Vec3i(40, 92, 186),
    cv::Vec3i(40, 184, 74),
    cv::Vec3i(40, 185, 184),
    cv::Vec3i(40, 186, 185),
    cv::Vec3i(41, 42, 81),
    cv::Vec3i(41, 73, 74),
    cv::Vec3i(41, 74, 42),
    cv::Vec3i(41, 81, 82),
    cv::Vec3i(42, 74, 184),
    cv::Vec3i(42, 80, 81),
    cv::Vec3i(42, 183, 80),
    cv::Vec3i(42, 184, 183),
    cv::Vec3i(43, 57, 212),
    cv::Vec3i(43, 91, 146),
    cv::Vec3i(43, 106, 91),
    cv::Vec3i(43, 146, 57),
    cv::Vec3i(43, 202, 106),
    cv::Vec3i(43, 212, 202),
    cv::Vec3i(44, 45, 220),
    cv::Vec3i(44, 220, 237),
    cv::Vec3i(44, 237, 241),
    cv::Vec3i(44, 241, 125),
    cv::Vec3i(45, 51, 134),
    cv::Vec3i(45, 134, 220),
    cv::Vec3i(46, 53, 63),
    cv::Vec3i(46, 63, 70),
    cv::Vec3i(46, 70, 124),
    cv::Vec3i(46, 113, 225),
    cv::Vec3i(46, 124, 113),
    cv::Vec3i(46, 225, 53),
    cv::Vec3i(47, 100, 126),
    cv::Vec3i(47, 114, 121),
    cv::Vec3i(47, 120, 100),
    cv::Vec3i(47, 121, 120),
    cv::Vec3i(47, 126, 217),
    cv::Vec3i(47, 217, 114),
    cv::Vec3i(48, 49, 102),
    cv::Vec3i(48, 64, 219),
    cv::Vec3i(48, 102, 64),
    cv::Vec3i(48, 115, 131),
    cv::Vec3i(48, 131, 49),
    cv::Vec3i(48, 219, 115),
    cv::Vec3i(49, 129, 102),
    cv::Vec3i(49, 131, 198),
    cv::Vec3i(49, 198, 209),
    cv::Vec3i(49, 209, 129),
    cv::Vec3i(50, 101, 118),
    cv::Vec3i(50, 117, 123),
    cv::Vec3i(50, 118, 117),
    cv::Vec3i(50, 123, 187),
    cv::Vec3i(50, 187, 205),
    cv::Vec3i(50, 205, 101),
    cv::Vec3i(51, 236, 134),
    cv::Vec3i(52, 53, 224),
    cv::Vec3i(52, 65, 66),
    cv::Vec3i(52, 66, 105),
    cv::Vec3i(52, 105, 53),
    cv::Vec3i(52, 222, 65),
    cv::Vec3i(52, 223, 222),
    cv::Vec3i(52, 224, 223),
    cv::Vec3i(53, 105, 63),
    cv::Vec3i(53, 225, 224),
    cv::Vec3i(54, 68, 103),
    cv::Vec3i(54, 71, 68),
    cv::Vec3i(55, 65, 222),
    cv::Vec3i(55, 107, 65),
    cv::Vec3i(55, 221, 193),
    cv::Vec3i(55, 222, 221),
    cv::Vec3i(56, 157, 190),
    cv::Vec3i(56, 158, 157),
    cv::Vec3i(56, 189, 221),
    cv::Vec3i(56, 190, 189),
    cv::Vec3i(56, 221, 222),
    cv::Vec3i(57, 61, 185),
    cv::Vec3i(57, 146, 61),
    cv::Vec3i(57, 185, 186),
    cv::Vec3i(57, 186, 212),
    cv::Vec3i(58, 172, 215),
    cv::Vec3i(58, 177, 132),
    cv::Vec3i(58, 215, 177),
    cv::Vec3i(59, 75, 166),
    cv::Vec3i(59, 166, 219),
    cv::Vec3i(59, 219, 235),
    cv::Vec3i(59, 235, 75),
    cv::Vec3i(60, 75, 99),
    cv::Vec3i(60, 79, 166),
    cv::Vec3i(60, 166, 75),
    cv::Vec3i(61, 76, 185),
    cv::Vec3i(61, 146, 76),
    cv::Vec3i(62, 76, 77),
    cv::Vec3i(62, 77, 96),
    cv::Vec3i(62, 78, 191),
    cv::Vec3i(62, 95, 78),
    cv::Vec3i(62, 96, 95),
    cv::Vec3i(62, 183, 76),
    cv::Vec3i(62, 191, 183),
    cv::Vec3i(63, 68, 70),
    cv::Vec3i(63, 104, 68),
    cv::Vec3i(63, 105, 104),
    cv::Vec3i(64, 98, 240),
    cv::Vec3i(64, 102, 129),
    cv::Vec3i(64, 129, 98),
    cv::Vec3i(64, 235, 219),
    cv::Vec3i(64, 240, 235),
    cv::Vec3i(65, 107, 66),
    cv::Vec3i(66, 69, 105),
    cv::Vec3i(66, 107, 108),
    cv::Vec3i(66, 108, 69),
    cv::Vec3i(67, 69, 109),
    cv::Vec3i(67, 103, 104),
    cv::Vec3i(67, 104, 69),
    cv::Vec3i(68, 71, 70),
    cv::Vec3i(68, 104, 103),
    cv::Vec3i(69, 104, 105),
    cv::Vec3i(69, 108, 109),
    cv::Vec3i(70, 71, 156),
    cv::Vec3i(70, 156, 124),
    cv::Vec3i(71, 139, 156),
    cv::Vec3i(75, 235, 240),
    cv::Vec3i(75, 240, 99),
    cv::Vec3i(76, 146, 77),
    cv::Vec3i(76, 183, 184),
    cv::Vec3i(76, 184, 185),
    cv::Vec3i(77, 90, 96),
    cv::Vec3i(77, 91, 90),
    cv::Vec3i(77, 146, 91),
    cv::Vec3i(78, 95, 191),
    cv::Vec3i(79, 218, 166),
    cv::Vec3i(79, 238, 239),
    cv::Vec3i(79, 239, 218),
    cv::Vec3i(80, 88, 81),
    cv::Vec3i(80, 95, 88),
    cv::Vec3i(80, 183, 191),
    cv::Vec3i(80, 191, 95),
    cv::Vec3i(81, 88, 178),
    cv::Vec3i(81, 178, 82),
    cv::Vec3i(82, 178, 87),
    cv::Vec3i(83, 84, 182),
    cv::Vec3i(83, 182, 201),
    cv::Vec3i(83, 201, 200),
    cv::Vec3i(84, 85, 181),
    cv::Vec3i(84, 181, 182),
    cv::Vec3i(85, 86, 180),
    cv::Vec3i(85, 180, 181),
    cv::Vec3i(86, 87, 179),
    cv::Vec3i(86, 179, 180),
    cv::Vec3i(87, 178, 179),
    cv::Vec3i(88, 89, 179),
    cv::Vec3i(88, 95, 96),
    cv::Vec3i(88, 96, 89),
    cv::Vec3i(88, 179, 178),
    cv::Vec3i(89, 90, 179),
    cv::Vec3i(89, 96, 90),
    cv::Vec3i(90, 91, 180),
    cv::Vec3i(90, 180, 179),
    cv::Vec3i(91, 106, 182),
    cv::Vec3i(91, 181, 180),
    cv::Vec3i(91, 182, 181),
    cv::Vec3i(92, 165, 206),
    cv::Vec3i(92, 206, 216),
    cv::Vec3i(92, 216, 186),
    cv::Vec3i(93, 132, 177),
    cv::Vec3i(93, 137, 234),
    cv::Vec3i(93, 177, 137),
    cv::Vec3i(94, 125, 141),
    cv::Vec3i(94, 370, 354),
    cv::Vec3i(97, 98, 167),
    cv::Vec3i(97, 99, 240),
    cv::Vec3i(97, 141, 242),
    cv::Vec3i(97, 167, 164),
    cv::Vec3i(97, 240, 98),
    cv::Vec3i(97, 242, 99),
    cv::Vec3i(98, 129, 203),
    cv::Vec3i(98, 165, 167),
    cv::Vec3i(98, 203, 165),
    cv::Vec3i(100, 119, 101),
    cv::Vec3i(100, 120, 119),
    cv::Vec3i(100, 142, 126),
    cv::Vec3i(101, 119, 118),
    cv::Vec3i(106, 202, 204),
    cv::Vec3i(106, 204, 182),
    cv::Vec3i(107, 151, 108),
    cv::Vec3i(110, 144, 163),
    cv::Vec3i(110, 228, 229),
    cv::Vec3i(111, 116, 117),
    cv::Vec3i(111, 143, 116),
    cv::Vec3i(112, 133, 155),
    cv::Vec3i(112, 232, 233),
    cv::Vec3i(112, 233, 243),
    cv::Vec3i(112, 243, 133),
    cv::Vec3i(113, 124, 226),
    cv::Vec3i(113, 130, 247),
    cv::Vec3i(113, 226, 130),
    cv::Vec3i(113, 247, 225),
    cv::Vec3i(114, 128, 121),
    cv::Vec3i(114, 174, 188),
    cv::Vec3i(114, 188, 245),
    cv::Vec3i(114, 217, 174),
    cv::Vec3i(114, 245, 128),
    cv::Vec3i(115, 218, 220),
    cv::Vec3i(115, 219, 218),
    cv::Vec3i(115, 220, 131),
    cv::Vec3i(116, 123, 117),
    cv::Vec3i(116, 227, 123),
    cv::Vec3i(117, 118, 228),
    cv::Vec3i(118, 119, 229),
    cv::Vec3i(118, 229, 228),
    cv::Vec3i(119, 120, 230),
    cv::Vec3i(119, 230, 229),
    cv::Vec3i(120, 121, 231),
    cv::Vec3i(120, 231, 230),
    cv::Vec3i(121, 128, 232),
    cv::Vec3i(121, 232, 231),
    cv::Vec3i(122, 168, 193),
    cv::Vec3i(122, 188, 196),
    cv::Vec3i(122, 193, 245),
    cv::Vec3i(122, 245, 188),
    cv::Vec3i(123, 137, 147),
    cv::Vec3i(123, 147, 187),
    cv::Vec3i(123, 227, 137),
    cv::Vec3i(125, 241, 141),
    cv::Vec3i(126, 142, 209),
    cv::Vec3i(126, 198, 217),
    cv::Vec3i(126, 209, 198),
    cv::Vec3i(127, 234, 227),
    cv::Vec3i(128, 233, 232),
    cv::Vec3i(128, 244, 233),
    cv::Vec3i(128, 245, 244),
    cv::Vec3i(129, 209, 142),
    cv::Vec3i(131, 134, 198),
    cv::Vec3i(131, 220, 134),
    cv::Vec3i(133, 173, 155),
    cv::Vec3i(133, 243, 173),
    cv::Vec3i(134, 236, 198),
    cv::Vec3i(135, 136, 150),
    cv::Vec3i(135, 138, 136),
    cv::Vec3i(135, 150, 169),
    cv::Vec3i(135, 169, 210),
    cv::Vec3i(135, 192, 138),
    cv::Vec3i(135, 210, 214),
    cv::Vec3i(135, 214, 192),
    cv::Vec3i(136, 138, 172),
    cv::Vec3i(137, 177, 147),
    cv::Vec3i(137, 227, 234),
    cv::Vec3i(138, 192, 215),
    cv::Vec3i(138, 215, 172),
    cv::Vec3i(139, 143, 156),
    cv::Vec3i(140, 149, 176),
    cv::Vec3i(140, 170, 149),
    cv::Vec3i(140, 171, 208),
    cv::Vec3i(140, 176, 171),
    cv::Vec3i(141, 241, 242),
    cv::Vec3i(144, 145, 160),
    cv::Vec3i(144, 160, 161),
    cv::Vec3i(144, 161, 163),
    cv::Vec3i(145, 153, 159),
    cv::Vec3i(145, 159, 160),
    cv::Vec3i(147, 177, 213),
    cv::Vec3i(147, 192, 187),
    cv::Vec3i(147, 213, 192),
    cv::Vec3i(148, 152, 171),
    cv::Vec3i(148, 171, 176),
    cv::Vec3i(149, 169, 150),
    cv::Vec3i(149, 170, 169),
    cv::Vec3i(151, 336, 337),
    cv::Vec3i(152, 175, 171),
    cv::Vec3i(152, 377, 396),
    cv::Vec3i(152, 396, 175),
    cv::Vec3i(153, 154, 158),
    cv::Vec3i(153, 158, 159),
    cv::Vec3i(154, 155, 157),
    cv::Vec3i(154, 157, 158),
    cv::Vec3i(155, 173, 157),
    cv::Vec3i(157, 173, 190),
    cv::Vec3i(161, 246, 163),
    cv::Vec3i(161, 247, 246),
    cv::Vec3i(164, 267, 393),
    cv::Vec3i(164, 393, 326),
    cv::Vec3i(165, 203, 206),
    cv::Vec3i(166, 218, 219),
    cv::Vec3i(168, 351, 417),
    cv::Vec3i(169, 170, 210),
    cv::Vec3i(170, 211, 210),
    cv::Vec3i(171, 175, 208),
    cv::Vec3i(173, 243, 190),
    cv::Vec3i(174, 196, 188),
    cv::Vec3i(174, 198, 236),
    cv::Vec3i(174, 217, 198),
    cv::Vec3i(174, 236, 196),
    cv::Vec3i(175, 199, 208),
    cv::Vec3i(175, 396, 428),
    cv::Vec3i(175, 428, 199),
    cv::Vec3i(177, 215, 213),
    cv::Vec3i(182, 194, 201),
    cv::Vec3i(182, 204, 194),
    cv::Vec3i(186, 216, 212),
    cv::Vec3i(187, 192, 207),
    cv::Vec3i(187, 207, 205),
    cv::Vec3i(189, 190, 244),
    cv::Vec3i(189, 193, 221),
    cv::Vec3i(189, 244, 245),
    cv::Vec3i(189, 245, 193),
    cv::Vec3i(190, 243, 244),
    cv::Vec3i(192, 213, 215),
    cv::Vec3i(192, 214, 207),
    cv::Vec3i(194, 204, 211),
    cv::Vec3i(195, 248, 197),
    cv::Vec3i(195, 281, 248),
    cv::Vec3i(197, 248, 419),
    cv::Vec3i(199, 200, 208),
    cv::Vec3i(199, 428, 200),
    cv::Vec3i(200, 201, 208),
    cv::Vec3i(200, 421, 313),
    cv::Vec3i(200, 428, 421),
    cv::Vec3i(202, 210, 211),
    cv::Vec3i(202, 211, 204),
    cv::Vec3i(202, 212, 210),
    cv::Vec3i(205, 207, 206),
    cv::Vec3i(206, 207, 216),
    cv::Vec3i(207, 214, 216),
    cv::Vec3i(210, 212, 214),
    cv::Vec3i(212, 216, 214),
    cv::Vec3i(218, 237, 220),
    cv::Vec3i(218, 239, 237),
    cv::Vec3i(233, 244, 243),
    cv::Vec3i(237, 239, 241),
    cv::Vec3i(238, 241, 239),
    cv::Vec3i(238, 242, 241),
    cv::Vec3i(248, 281, 456),
    cv::Vec3i(248, 456, 419),
    cv::Vec3i(249, 255, 263),
    cv::Vec3i(249, 263, 466),
    cv::Vec3i(249, 339, 255),
    cv::Vec3i(249, 390, 339),
    cv::Vec3i(249, 466, 390),
    cv::Vec3i(250, 290, 309),
    cv::Vec3i(250, 309, 458),
    cv::Vec3i(250, 328, 290),
    cv::Vec3i(250, 458, 462),
    cv::Vec3i(250, 462, 328),
    cv::Vec3i(251, 284, 301),
    cv::Vec3i(251, 301, 368),
    cv::Vec3i(251, 368, 389),
    cv::Vec3i(252, 253, 380),
    cv::Vec3i(252, 256, 452),
    cv::Vec3i(252, 380, 381),
    cv::Vec3i(252, 381, 256),
    cv::Vec3i(252, 451, 253),
    cv::Vec3i(252, 452, 451),
    cv::Vec3i(253, 254, 373),
    cv::Vec3i(253, 373, 374),
    cv::Vec3i(253, 374, 380),
    cv::Vec3i(253, 450, 254),
    cv::Vec3i(253, 451, 450),
    cv::Vec3i(254, 339, 373),
    cv::Vec3i(254, 449, 339),
    cv::Vec3i(254, 450, 449),
    cv::Vec3i(255, 261, 446),
    cv::Vec3i(255, 339, 448),
    cv::Vec3i(255, 359, 263),
    cv::Vec3i(255, 446, 359),
    cv::Vec3i(255, 448, 261),
    cv::Vec3i(256, 341, 452),
    cv::Vec3i(256, 381, 382),
    cv::Vec3i(256, 382, 341),
    cv::Vec3i(257, 258, 386),
    cv::Vec3i(257, 259, 443),
    cv::Vec3i(257, 386, 259),
    cv::Vec3i(257, 443, 258),
    cv::Vec3i(258, 286, 385),
    cv::Vec3i(258, 385, 386),
    cv::Vec3i(258, 442, 286),
    cv::Vec3i(258, 443, 442),
    cv::Vec3i(259, 260, 444),
    cv::Vec3i(259, 386, 387),
    cv::Vec3i(259, 387, 260),
    cv::Vec3i(259, 444, 443),
    cv::Vec3i(260, 387, 388),
    cv::Vec3i(260, 388, 467),
    cv::Vec3i(260, 445, 444),
    cv::Vec3i(260, 467, 445),
    cv::Vec3i(261, 265, 446),
    cv::Vec3i(261, 340, 265),
    cv::Vec3i(261, 346, 340),
    cv::Vec3i(261, 448, 346),
    cv::Vec3i(262, 369, 395),
    cv::Vec3i(262, 395, 431),
    cv::Vec3i(262, 418, 421),
    cv::Vec3i(262, 421, 428),
    cv::Vec3i(262, 428, 369),
    cv::Vec3i(262, 431, 418),
    cv::Vec3i(263, 359, 467),
    cv::Vec3i(263, 467, 466),
    cv::Vec3i(264, 345, 447),
    cv::Vec3i(264, 356, 389),
    cv::Vec3i(264, 368, 372),
    cv::Vec3i(264, 372, 345),
    cv::Vec3i(264, 389, 368),
    cv::Vec3i(264, 447, 356),
    cv::Vec3i(265, 340, 372),
    cv::Vec3i(265, 353, 446),
    cv::Vec3i(265, 372, 383),
    cv::Vec3i(265, 383, 353),
    cv::Vec3i(266, 329, 371),
    cv::Vec3i(266, 330, 329),
    cv::Vec3i(266, 358, 423),
    cv::Vec3i(266, 371, 358),
    cv::Vec3i(266, 423, 426),
    cv::Vec3i(266, 425, 330),
    cv::Vec3i(266, 426, 425),
    cv::Vec3i(267, 269, 393),
    cv::Vec3i(267, 302, 269),
    cv::Vec3i(268, 271, 303),
    cv::Vec3i(268, 303, 302),
    cv::Vec3i(268, 312, 271),
    cv::Vec3i(269, 270, 322),
    cv::Vec3i(269, 302, 303),
    cv::Vec3i(269, 303, 270),
    cv::Vec3i(269, 322, 391),
    cv::Vec3i(269, 391, 393),
    cv::Vec3i(270, 303, 304),
    cv::Vec3i(270, 304, 408),
    cv::Vec3i(270, 408, 409),
    cv::Vec3i(270, 409, 410),
    cv::Vec3i(270, 410, 322),
    cv::Vec3i(271, 272, 304),
    cv::Vec3i(271, 304, 303),
    cv::Vec3i(271, 311, 272),
    cv::Vec3i(271, 312, 311),
    cv::Vec3i(272, 310, 407),
    cv::Vec3i(272, 311, 310),
    cv::Vec3i(272, 407, 408),
    cv::Vec3i(272, 408, 304),
    cv::Vec3i(273, 287, 375),
    cv::Vec3i(273, 321, 335),
    cv::Vec3i(273, 335, 422),
    cv::Vec3i(273, 375, 321),
    cv::Vec3i(273, 422, 432),
    cv::Vec3i(273, 432, 287),
    cv::Vec3i(274, 354, 461),
    cv::Vec3i(274, 440, 275),
    cv::Vec3i(274, 457, 440),
    cv::Vec3i(274, 461, 457),
    cv::Vec3i(275, 363, 281),
    cv::Vec3i(275, 440, 363),
    cv::Vec3i(276, 283, 445),
    cv::Vec3i(276, 293, 283),
    cv::Vec3i(276, 300, 293),
    cv::Vec3i(276, 342, 353),
    cv::Vec3i(276, 353, 300),
    cv::Vec3i(276, 445, 342),
    cv::Vec3i(277, 329, 349),
    cv::Vec3i(277, 343, 437),
    cv::Vec3i(277, 349, 350),
    cv::Vec3i(277, 350, 343),
    cv::Vec3i(277, 355, 329),
    cv::Vec3i(277, 437, 355),
    cv::Vec3i(278, 279, 360),
    cv::Vec3i(278, 294, 331),
    cv::Vec3i(278, 331, 279),
    cv::Vec3i(278, 344, 439),
    cv::Vec3i(278, 360, 344),
    cv::Vec3i(278, 439, 294),
    cv::Vec3i(279, 331, 358),
    cv::Vec3i(279, 358, 429),
    cv::Vec3i(279, 420, 360),
    cv::Vec3i(279, 429, 420),
    cv::Vec3i(280, 330, 425),
    cv::Vec3i(280, 346, 347),
    cv::Vec3i(280, 347, 330),
    cv::Vec3i(280, 352, 346),
    cv::Vec3i(280, 411, 352),
    cv::Vec3i(280, 425, 411),
    cv::Vec3i(281, 363, 456),
    cv::Vec3i(282, 283, 334),
    cv::Vec3i(282, 295, 442),
    cv::Vec3i(282, 296, 295),
    cv::Vec3i(282, 334, 296),
    cv::Vec3i(282, 442, 443),
    cv::Vec3i(282, 443, 444),
    cv::Vec3i(282, 444, 283),
    cv::Vec3i(283, 293, 334),
    cv::Vec3i(283, 444, 445),
    cv::Vec3i(284, 298, 301),
    cv::Vec3i(284, 332, 298),
    cv::Vec3i(285, 295, 336),
    cv::Vec3i(285, 417, 441),
    cv::Vec3i(285, 441, 442),
    cv::Vec3i(285, 442, 295),
    cv::Vec3i(286, 384, 385),
    cv::Vec3i(286, 413, 414),
    cv::Vec3i(286, 414, 384),
    cv::Vec3i(286, 441, 413),
    cv::Vec3i(286, 442, 441),
    cv::Vec3i(287, 291, 375),
    cv::Vec3i(287, 409, 291),
    cv::Vec3i(287, 410, 409),
    cv::Vec3i(287, 432, 410),
    cv::Vec3i(288, 361, 401),
    cv::Vec3i(288, 401, 435),
    cv::Vec3i(288, 435, 397),
    cv::Vec3i(289, 305, 455),
    cv::Vec3i(289, 392, 305),
    cv::Vec3i(289, 439, 392),
    cv::Vec3i(289, 455, 439),
    cv::Vec3i(290, 305, 392),
    cv::Vec3i(290, 328, 305),
    cv::Vec3i(290, 392, 309),
    cv::Vec3i(291, 306, 375),
    cv::Vec3i(291, 409, 306),
    cv::Vec3i(292, 306, 407),
    cv::Vec3i(292, 307, 306),
    cv::Vec3i(292, 308, 324),
    cv::Vec3i(292, 324, 325),
    cv::Vec3i(292, 325, 307),
    cv::Vec3i(292, 407, 415),
    cv::Vec3i(292, 415, 308),
    cv::Vec3i(293, 298, 333),
    cv::Vec3i(293, 300, 298),
    cv::Vec3i(293, 333, 334),
    cv::Vec3i(294, 327, 358),
    cv::Vec3i(294, 358, 331),
    cv::Vec3i(294, 439, 455),
    cv::Vec3i(294, 455, 460),
    cv::Vec3i(294, 460, 327),
    cv::Vec3i(295, 296, 336),
    cv::Vec3i(296, 299, 337),
    cv::Vec3i(296, 334, 299),
    cv::Vec3i(296, 337, 336),
    cv::Vec3i(297, 299, 333),
    cv::Vec3i(297, 333, 332),
    cv::Vec3i(297, 338, 299),
    cv::Vec3i(298, 300, 301),
    cv::Vec3i(298, 332, 333),
    cv::Vec3i(299, 334, 333),
    cv::Vec3i(299, 338, 337),
    cv::Vec3i(300, 353, 383),
    cv::Vec3i(300, 383, 301),
    cv::Vec3i(301, 383, 368),
    cv::Vec3i(305, 328, 460),
    cv::Vec3i(305, 460, 455),
    cv::Vec3i(306, 307, 375),
    cv::Vec3i(306, 408, 407),
    cv::Vec3i(306, 409, 408),
    cv::Vec3i(307, 320, 321),
    cv::Vec3i(307, 321, 375),
    cv::Vec3i(307, 325, 320),
    cv::Vec3i(308, 415, 324),
    cv::Vec3i(309, 392, 438),
    cv::Vec3i(309, 438, 459),
    cv::Vec3i(309, 459, 458),
    cv::Vec3i(310, 311, 318),
    cv::Vec3i(310, 318, 324),
    cv::Vec3i(310, 324, 415),
    cv::Vec3i(310, 415, 407),
    cv::Vec3i(311, 312, 402),
    cv::Vec3i(311, 402, 318),
    cv::Vec3i(312, 317, 402),
    cv::Vec3i(313, 406, 314),
    cv::Vec3i(313, 421, 406),
    cv::Vec3i(314, 405, 315),
    cv::Vec3i(314, 406, 405),
    cv::Vec3i(315, 404, 316),
    cv::Vec3i(315, 405, 404),
    cv::Vec3i(316, 403, 317),
    cv::Vec3i(316, 404, 403),
    cv::Vec3i(317, 403, 402),
    cv::Vec3i(318, 319, 325),
    cv::Vec3i(318, 325, 324),
    cv::Vec3i(318, 402, 403),
    cv::Vec3i(318, 403, 319),
    cv::Vec3i(319, 320, 325),
    cv::Vec3i(319, 403, 320),
    cv::Vec3i(320, 403, 404),
    cv::Vec3i(320, 404, 321),
    cv::Vec3i(321, 404, 405),
    cv::Vec3i(321, 405, 406),
    cv::Vec3i(321, 406, 335),
    cv::Vec3i(322, 410, 436),
    cv::Vec3i(322, 426, 391),
    cv::Vec3i(322, 436, 426),
    cv::Vec3i(323, 366, 401),
    cv::Vec3i(323, 401, 361),
    cv::Vec3i(323, 454, 366),
    cv::Vec3i(326, 327, 460),
    cv::Vec3i(326, 328, 462),
    cv::Vec3i(326, 393, 327),
    cv::Vec3i(326, 460, 328),
    cv::Vec3i(326, 462, 370),
    cv::Vec3i(327, 391, 423),
    cv::Vec3i(327, 393, 391),
    cv::Vec3i(327, 423, 358),
    cv::Vec3i(329, 330, 348),
    cv::Vec3i(329, 348, 349),
    cv::Vec3i(329, 355, 371),
    cv::Vec3i(330, 347, 348),
    cv::Vec3i(335, 406, 424),
    cv::Vec3i(335, 424, 422),
    cv::Vec3i(339, 390, 373),
    cv::Vec3i(339, 449, 448),
    cv::Vec3i(340, 345, 372),
    cv::Vec3i(340, 346, 345),
    cv::Vec3i(341, 362, 463),
    cv::Vec3i(341, 382, 362),
    cv::Vec3i(341, 453, 452),
    cv::Vec3i(341, 463, 453),
    cv::Vec3i(342, 359, 446),
    cv::Vec3i(342, 445, 467),
    cv::Vec3i(342, 446, 353),
    cv::Vec3i(342, 467, 359),
    cv::Vec3i(343, 350, 357),
    cv::Vec3i(343, 357, 465),
    cv::Vec3i(343, 399, 437),
    cv::Vec3i(343, 412, 399),
    cv::Vec3i(343, 465, 412),
    cv::Vec3i(344, 360, 440),
    cv::Vec3i(344, 438, 439),
    cv::Vec3i(344, 440, 438),
    cv::Vec3i(345, 346, 352),
    cv::Vec3i(345, 352, 447),
    cv::Vec3i(346, 448, 347),
    cv::Vec3i(347, 448, 449),
    cv::Vec3i(347, 449, 348),
    cv::Vec3i(348, 449, 450),
    cv::Vec3i(348, 450, 349),
    cv::Vec3i(349, 450, 451),
    cv::Vec3i(349, 451, 350),
    cv::Vec3i(350, 451, 452),
    cv::Vec3i(350, 452, 357),
    cv::Vec3i(351, 412, 465),
    cv::Vec3i(351, 419, 412),
    cv::Vec3i(351, 465, 417),
    cv::Vec3i(352, 366, 447),
    cv::Vec3i(352, 376, 366),
    cv::Vec3i(352, 411, 376),
    cv::Vec3i(354, 370, 461),
    cv::Vec3i(355, 420, 429),
    cv::Vec3i(355, 429, 371),
    cv::Vec3i(355, 437, 420),
    cv::Vec3i(356, 447, 454),
    cv::Vec3i(357, 452, 453),
    cv::Vec3i(357, 453, 464),
    cv::Vec3i(357, 464, 465),
    cv::Vec3i(358, 371, 429),
    cv::Vec3i(360, 363, 440),
    cv::Vec3i(360, 420, 363),
    cv::Vec3i(362, 382, 398),
    cv::Vec3i(362, 398, 463),
    cv::Vec3i(363, 420, 456),
    cv::Vec3i(364, 365, 367),
    cv::Vec3i(364, 367, 416),
    cv::Vec3i(364, 379, 365),
    cv::Vec3i(364, 394, 379),
    cv::Vec3i(364, 416, 434),
    cv::Vec3i(364, 430, 394),
    cv::Vec3i(364, 434, 430),
    cv::Vec3i(365, 397, 367),
    cv::Vec3i(366, 376, 401),
    cv::Vec3i(366, 454, 447),
    cv::Vec3i(367, 397, 435),
    cv::Vec3i(367, 435, 416),
    cv::Vec3i(368, 383, 372),
    cv::Vec3i(369, 378, 395),
    cv::Vec3i(369, 396, 400),
    cv::Vec3i(369, 400, 378),
    cv::Vec3i(369, 428, 396),
    cv::Vec3i(370, 462, 461),
    cv::Vec3i(373, 387, 374),
    cv::Vec3i(373, 388, 387),
    cv::Vec3i(373, 390, 388),
    cv::Vec3i(374, 386, 380),
    cv::Vec3i(374, 387, 386),
    cv::Vec3i(376, 411, 416),
    cv::Vec3i(376, 416, 433),
    cv::Vec3i(376, 433, 401),
    cv::Vec3i(377, 400, 396),
    cv::Vec3i(378, 379, 394),
    cv::Vec3i(378, 394, 395),
    cv::Vec3i(380, 385, 381),
    cv::Vec3i(380, 386, 385),
    cv::Vec3i(381, 384, 382),
    cv::Vec3i(381, 385, 384),
    cv::Vec3i(382, 384, 398),
    cv::Vec3i(384, 414, 398),
    cv::Vec3i(388, 390, 466),
    cv::Vec3i(388, 466, 467),
    cv::Vec3i(391, 426, 423),
    cv::Vec3i(392, 439, 438),
    cv::Vec3i(394, 430, 395),
    cv::Vec3i(395, 430, 431),
    cv::Vec3i(398, 414, 463),
    cv::Vec3i(399, 412, 419),
    cv::Vec3i(399, 419, 456),
    cv::Vec3i(399, 420, 437),
    cv::Vec3i(399, 456, 420),
    cv::Vec3i(401, 433, 435),
    cv::Vec3i(406, 418, 424),
    cv::Vec3i(406, 421, 418),
    cv::Vec3i(410, 432, 436),
    cv::Vec3i(411, 425, 427),
    cv::Vec3i(411, 427, 416),
    cv::Vec3i(413, 417, 465),
    cv::Vec3i(413, 441, 417),
    cv::Vec3i(413, 464, 414),
    cv::Vec3i(413, 465, 464),
    cv::Vec3i(414, 464, 463),
    cv::Vec3i(416, 427, 434),
    cv::Vec3i(416, 435, 433),
    cv::Vec3i(418, 431, 424),
    cv::Vec3i(422, 424, 431),
    cv::Vec3i(422, 430, 432),
    cv::Vec3i(422, 431, 430),
    cv::Vec3i(425, 426, 427),
    cv::Vec3i(426, 436, 427),
    cv::Vec3i(427, 436, 434),
    cv::Vec3i(430, 434, 432),
    cv::Vec3i(432, 434, 436),
    cv::Vec3i(438, 440, 457),
    cv::Vec3i(438, 457, 459),
    cv::Vec3i(453, 463, 464),
    cv::Vec3i(457, 461, 459),
    cv::Vec3i(458, 459, 461),
    cv::Vec3i(458, 461, 462),
};

    FaceLandmarks::FaceLandmarks(std::unique_ptr<Ort::Env> &ort_env) {
        unsigned long tic = get_time_ms();
        if (!ortSession) {
            Ort::SessionOptions sessionOptions;
            sessionOptions.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
            sessionOptions.EnableCpuMemArena();
            sessionOptions.EnableMemPattern();
            sessionOptions.DisablePerSessionThreads();
            sessionOptions.SetExecutionMode(ExecutionMode::ORT_PARALLEL);
            //const char *model_data_path = obs_module_file("face_detection_models/face_landmarks_detector.onnx");
            char *model_data_path = obs_module_file("face_detection_models/face_landmark_with_attention_192x192.onnx");
            //const char *model_data_path = obs_module_file("face_detection_models/blazeface.onnx");

            if (model_data_path) {
                #if defined(_WIN32)
                    std::string model_data_path_ = std::string(model_data_path);
                    std::wstring model_data_path__ = std::wstring(model_data_path_.begin(), model_data_path_.end());
                    info("MODEL DATA PATH: %s", model_data_path_.c_str());
                    info("MODEL DATA PATH: %s", model_data_path_.c_str());
                    info("MODEL DATA PATH: %s", model_data_path_.c_str());
                    ortSession = std::make_shared<Ort::Session>(*ort_env, (const ORTCHAR_T*)(model_data_path__.c_str()), sessionOptions);
                #else
                    ortSession = std::make_shared<Ort::Session>(*ort_env, (const ORTCHAR_T*)model_data_path, sessionOptions);
                #endif
                bfree(model_data_path);
                debug("FACE_LANDMARKS Loading model %s", model_data_path);
            }
        }
        if (!ortSession) {
            return;
        }

        // Prepare inputs / outputs
        {
            if (ortSession->GetInputCount() != 1) {
                throw std::logic_error("FaceDetection model topology should have only 1 input");
            }

            inputCount = ortSession->GetInputCount();
            outputCount = ortSession->GetOutputCount();
            debug("FACE_LANDMARKS Inputs: %lu", inputCount);
            debug("FACE_LANDMARKS Outputs: %lu", outputCount);

            Ort::AllocatorWithDefaultOptions allocator;
            Ort::MemoryInfo memoryInfo = Ort::MemoryInfo::CreateCpu(OrtAllocatorType::OrtArenaAllocator, OrtMemType::OrtMemTypeDefault);

            std::vector<std::vector<int64_t>> inputShapes(inputCount);
            for (size_t i = 0; i < inputCount; ++i) {
                std::string inputName = ortSession->GetInputNameAllocated(i, allocator).get();
                char *inputNameCStr = new char[inputName.length()+1];
                strcpy(inputNameCStr, inputName.c_str());
                inputNames.push_back(inputNameCStr);
                debug("FACE_LANDMARKS Input Name %lu: %s", i, inputNameCStr);
                std::vector<int64_t> inputShape = ortSession->GetInputTypeInfo(i).GetTensorTypeAndShapeInfo().GetShape();
                if (inputShape[0] < 0) {
                    //Negative batch size, aka dynamic. We'll only use one
                    inputShape[0] = 1;
                }
                auto elementType = ortSession->GetInputTypeInfo(i).GetTensorTypeAndShapeInfo().GetElementType();
                inputShapes[i] = inputShape;
                debug("FACE_LANDMARKS Input Shape %lu: %s", i, printShape(inputShape).c_str());
                debug("FACE_LANDMARKS Input Type %lu: %s", i, printElementType(elementType));
                int64_t inputTensorSize = vectorProduct(inputShape);
                inputTensorValues.push_back(std::vector<float>((size_t)inputTensorSize));

                inputTensors.push_back(Ort::Value::CreateTensor<float>(
                    memoryInfo,
                    inputTensorValues[i].data(), (size_t)inputTensorSize,
                    inputShape.data(), inputShape.size()
                ));
            }

            std::vector<std::vector<int64_t>> outputShapes(outputCount);
            for (size_t i = 0; i < outputCount; ++i) {
                std::string outputName = ortSession->GetOutputNameAllocated(i, allocator).get();
                char *outputNameCStr = new char[outputName.length()+1];
                strcpy(outputNameCStr, outputName.c_str());
                outputNames.push_back(outputNameCStr);
                debug("FACE_LANDMARKS Output Name %lu: %s", i, outputNameCStr);
                std::vector<int64_t> outputShape = ortSession->GetOutputTypeInfo(i).GetTensorTypeAndShapeInfo().GetShape();
                if (outputShape[0] < 0) {
                    //Negative batch size, aka dynamic. We'll only use one
                    outputShape[0] = 1;
                }
                outputShapes[i] = outputShape;
                debug("FACE_LANDMARKS Output Shape %lu: %s", i, printShape(outputShape).c_str());
                size_t outputTensorSize = vectorProduct(outputShape);
                outputTensorValues.push_back(std::vector<float>(outputTensorSize));

                outputTensors.push_back(Ort::Value::CreateTensor<float>(
                    memoryInfo,
                    outputTensorValues[i].data(), outputTensorSize,
                    outputShape.data(), outputShape.size()
                ));
            }
            netInputHeight = inputShapes[0][2];
            netInputWidth = inputShapes[0][3];
        }

        unsigned long toc = get_time_ms();
        debug("FACE_LANDMARKS Done Loading model in %li ms", toc-tic);
    }

    void FaceLandmarks::Run(const cv::Mat& frameBGR, int image_width, int image_height, const RotatedRect& roi, FaceLandmarksResults& results) {
        #ifdef DEV_MODE
                unsigned long tic = get_time_us();
        #endif
        faceLandmarksDebugFile = fdebug_open("face_landmarks.txt");
        preprocess(frameBGR);
        debug_trace("  a %lu", get_time_us()-tic);

        // Perform inference
        /* To run inference, we provide the run options, an array of input names corresponding to the
        inputs in the input tensor, an array of input tensor, number of inputs, an array of output names
        corresponding to the outputs in the output tensor, an array of output tensor, number of outputs. */
        ortSession->Run(runOptions,
            inputNames.data(), inputTensors.data(), inputCount,
            outputNames.data(), outputTensors.data(), outputCount);
        debug_trace("  b %lu", get_time_us()-tic);

        // post-process
        postprocess(image_width, image_height, roi, results);
        fdebug_close(faceLandmarksDebugFile);
        debug_trace("  c %lu", get_time_us()-tic);
    }

    void FaceLandmarks::preprocess(const cv::Mat &frameBGR) {
        // Wrap the already-allocated tensor as a cv::Mat of floats
        float* pTensor = inputTensorValues[0].data();
        cv::Mat converted = cv::Mat((int)netInputHeight*3, (int)netInputWidth, CV_32FC1, pTensor);

        //hwcToChw
        std::vector<cv::Mat> channels;
        cv::split(frameBGR, channels);
        // Concatenate three vectors to one
        cv::vconcat(channels, converted);
    }

    static inline void fill2d_points_results(const float* raw_tensor, const size_t num_points, cv::Point2f v[], const int netInputWidth, const int netInputHeight) {
        for (size_t i = 0; i < num_points; i++) {
            v[i].x = raw_tensor[i * 2] / (float)netInputWidth;
            v[i].y = raw_tensor[i * 2 + 1] / (float)netInputHeight;
        }
    }

    static inline void transform_point(cv::Point2f &p, const float cos_angle, const float sin_angle, const RotatedRect &rect) {
        const float x = p.x - 0.5f;
        const float y = p.y - 0.5f;
        const float new_x = cos_angle * x - sin_angle * y;
        const float new_y = sin_angle * x + cos_angle * y;

        p.x = new_x * rect.width + rect.center_x;
        p.y = new_y * rect.height + rect.center_y;
    }

    static inline float edge_function(const cv::Point2f& a, const cv::Point2f& b, const cv::Point2f& c) {
        return (c.x - a.x) * (b.y - a.y) - (c.y - a.y) * (b.x - a.x);
    }

    cv::Mat rasterize_face_mesh_uv(
        cv::Point3f uvs[],
        const cv::Vec3i triangles[],
        int width,
        int height
    ) {
        cv::Mat out(height, width, CV_32FC1, cv::Scalar(-1.0f));

        for (int tri_id = 0; tri_id < FACE_TRIANGLES_COUNT; ++tri_id) {
            const cv::Vec3i& tri = triangles[tri_id];

            cv::Point2f uv0 = cv::Point2f(uvs[tri[0]].x, uvs[tri[0]].y);
            cv::Point2f uv1 = cv::Point2f(uvs[tri[1]].x, uvs[tri[1]].y);
            cv::Point2f uv2 = cv::Point2f(uvs[tri[2]].x, uvs[tri[2]].y);

            // UV -> pixel space
            cv::Point2f p0(uv0.x * width,  uv0.y * height);
            cv::Point2f p1(uv1.x * width,  uv1.y * height);
            cv::Point2f p2(uv2.x * width,  uv2.y * height);

            // Bounding box
            int min_x = std::max(0, (int)std::floor(std::min({p0.x, p1.x, p2.x})));
            int max_x = std::min(width - 1, (int)std::ceil (std::max({p0.x, p1.x, p2.x})));

            int min_y = std::max(0, (int)std::floor(std::min({p0.y, p1.y, p2.y})));
            int max_y = std::min(height - 1, (int)std::ceil (std::max({p0.y, p1.y, p2.y})));

            float area = edge_function(p0, p1, p2);
            if (std::abs(area) < 1e-6f) {
                continue;
            }

            for (int y = min_y; y <= max_y; ++y) {
                for (int x = min_x; x <= max_x; ++x) {

                    cv::Point2f p((float)x + 0.5f, (float)y + 0.5f);

                    float w0 = edge_function(p1, p2, p);
                    float w1 = edge_function(p2, p0, p);
                    float w2 = edge_function(p0, p1, p);

                    // même orientation
                    if (w0 >= 0 && w1 >= 0 && w2 >= 0) {
                        out.at<float>(y, x) = (tri_id + 1.0) / (float)(FACE_TRIANGLES_COUNT);
                    }
                }
            }
        }

        return out;
    }

    void FaceLandmarks::postprocess(int image_width, int image_height, const RotatedRect& roi, FaceLandmarksResults& results) {
        results.face_flag = 0.f;

        const float* facial_surface_tensor_data = outputTensorValues[4].data();
        const float* face_flag_data = outputTensorValues[0].data();

//        //double check that the output tensors have the correct size
//        {
//            size_t facial_surface_tensor_size = inferRequest.get_tensor(facial_surface_tensor_name).get_byte_size();
//            if (facial_surface_tensor_size < (nFacialSurfaceLandmarks * 3 * sizeof(float)))
//            {
//                throw std::logic_error("facial surface tensor is holding a smaller amount of data than expected.");
//            }
//
//            size_t face_flag_tensor_size = inferRequest.get_tensor(face_flag_tensor_name).get_byte_size();
//            if (face_flag_tensor_size < (sizeof(float)))
//            {
//                throw std::logic_error("face flag tensor is holding a smaller amount of data than expected.");
//            }
//        }

        //apply sigmoid activation to produce face flag result
        results.face_flag = 1.0f / (1.0f + std::exp(-(*face_flag_data)));
        fdebug(faceLandmarksDebugFile, "Face Flag: %f → %f", *face_flag_data, results.face_flag);

        // Pre-compute common values
        const float inv_net_width = 1.0f / (float)netInputWidth;
        const float inv_net_height = 1.0f / (float)netInputHeight;

        // Process facial surface points - optimized for cache locality
        for (size_t i = 0; i < facial_surface_num_points; i++) {
            const size_t idx = i * 3;
            results.facial_surface[i].x = facial_surface_tensor_data[idx] * inv_net_width;
            results.facial_surface[i].y = facial_surface_tensor_data[idx + 1] * inv_net_height;
            results.facial_surface[i].z = facial_surface_tensor_data[idx + 2] * inv_net_width;
            fdebug(faceLandmarksDebugFile, "%i %f %f %f", i, results.facial_surface[i].x, results.facial_surface[i].y, results.facial_surface[i].z);
        }

        if (_bWithAttention) {
            const float* lips_refined_region_data = outputTensorValues[3].data();
            const float* left_eye_refined_region_data = outputTensorValues[1].data();
            const float* right_eye_refined_region_data = outputTensorValues[5].data();
            const float* left_iris_refined_region_data = outputTensorValues[2].data();
            const float* right_iris_refined_region_data = outputTensorValues[6].data();

//            //double check that the output tensors have the correct size
//            {
//                if (inferRequest.get_tensor(lips_refined_tensor_name).get_byte_size() < lips_refined_region_num_points * 2 * sizeof(float))
//                    throw std::logic_error(lips_refined_tensor_name + " output tensor is holding a smaller amount of data than expected.");
//
//                if (inferRequest.get_tensor(left_eye_with_eyebrow_tensor_name).get_byte_size() < left_eye_refined_region_num_points * 2 * sizeof(float))
//                    throw std::logic_error(left_eye_with_eyebrow_tensor_name + " output tensor is holding a smaller amount of data than expected.");
//
//                if (inferRequest.get_tensor(right_eye_with_eyebrow_tensor_name).get_byte_size() < right_eye_refined_region_num_points * 2 * sizeof(float))
//                    throw std::logic_error(right_eye_with_eyebrow_tensor_name + " output tensor is holding a smaller amount of data than expected.");
//
//                if (inferRequest.get_tensor(left_iris_refined_tensor_name).get_byte_size() < left_iris_refined_region_num_points * 2 * sizeof(float))
//                    throw std::logic_error(left_iris_refined_tensor_name + " output tensor is holding a smaller amount of data than expected.");
//
//                if (inferRequest.get_tensor(right_iris_refined_tensor_name).get_byte_size() < right_iris_refined_region_num_points * 2 * sizeof(float))
//                    throw std::logic_error(right_iris_refined_tensor_name + " output tensor is holding a smaller amount of data than expected.");
//            }

            fill2d_points_results(lips_refined_region_data,       lips_refined_region_num_points, results.lips_refined_region,       (int)netInputWidth, (int)netInputHeight);
            fill2d_points_results(left_eye_refined_region_data,   eye_refined_region_num_points,  results.left_eye_refined_region,   (int)netInputWidth, (int)netInputHeight);
            fill2d_points_results(right_eye_refined_region_data,  eye_refined_region_num_points,  results.right_eye_refined_region,  (int)netInputWidth, (int)netInputHeight);
            fill2d_points_results(left_iris_refined_region_data,  iris_refined_region_num_points, results.left_iris_refined_region,  (int)netInputWidth, (int)netInputHeight);
            fill2d_points_results(right_iris_refined_region_data, iris_refined_region_num_points, results.right_iris_refined_region, (int)netInputWidth, (int)netInputHeight);

            //create a (normalized) refined list of landmarks from the 6 separate lists that we generated.

            //initialize the first 468 points to our face surface landmarks
            memcpy(results.refined_landmarks, results.facial_surface, facial_surface_num_points * sizeof(cv::Point3f));

            //override x & y for lip points
            for (size_t i = 0; i < lips_refined_region_num_points; i++) {
                const size_t idx = lips_refinement_indices[i];
                results.refined_landmarks[idx].x = results.lips_refined_region[i].x;
                results.refined_landmarks[idx].y = results.lips_refined_region[i].y;
            }

            //override x & y for left & right_eye points
            for (size_t i = 0; i < eye_refined_region_num_points; i++) {
                const size_t right_idx = right_eye_refinement_indices[i];
                const size_t left_idx = left_eye_refinement_indices[i];

                results.refined_landmarks[right_idx].x = results.right_eye_refined_region[i].x;
                results.refined_landmarks[right_idx].y = results.right_eye_refined_region[i].y;

                results.refined_landmarks[left_idx].x = results.left_eye_refined_region[i].x;
                results.refined_landmarks[left_idx].y = results.left_eye_refined_region[i].y;
            }

            float z_avg_for_left_iris = 0.f;
            float z_avg_for_right_iris = 0.f;
            for (int i = 0; i < 16; i++) {
                z_avg_for_left_iris += results.refined_landmarks[left_iris_z_avg_indices[i]].z;
                z_avg_for_right_iris += results.refined_landmarks[right_iris_z_avg_indices[i]].z;
            }
            z_avg_for_left_iris *= 0.0625f;  // 0.0625 == 1.0/16.0
            z_avg_for_right_iris *= 0.0625f;

            //set x & y for left & right iris points
            for (size_t i = 0; i < iris_refined_region_num_points; i++) {
                const size_t left_idx = left_iris_refinement_indices[i];
                const size_t right_idx = right_iris_refinement_indices[i];

                results.refined_landmarks[left_idx].x = results.left_iris_refined_region[i].x;
                results.refined_landmarks[left_idx].y = results.left_iris_refined_region[i].y;
                results.refined_landmarks[left_idx].z = z_avg_for_left_iris;

                results.refined_landmarks[right_idx].x = results.right_iris_refined_region[i].x;
                results.refined_landmarks[right_idx].y = results.right_iris_refined_region[i].y;
                results.refined_landmarks[right_idx].z = z_avg_for_right_iris;
            }
        }

        //project the points back into the pre-rotated / pre-cropped space
        {
            const RotatedRect normalized_rect = roi;

            const float angle = normalized_rect.rotation;
            const float sin_angle = std::sin(angle);
            const float cos_angle = std::cos(angle);

            for (size_t i = 0; i < refined_landmarks_num_points; i++) {
                cv::Point3f &p = results.refined_landmarks[i];
                const float x = p.x - 0.5f;
                const float y = p.y - 0.5f;
                const float new_x = cos_angle * x - sin_angle * y;
                const float new_y = sin_angle * x + cos_angle * y;

                p.x = new_x * normalized_rect.width + normalized_rect.center_x;
                p.y = new_y * normalized_rect.height + normalized_rect.center_y;
                p.z = p.z * normalized_rect.width;  // Scale Z coordinate as X.
            }

            for (auto& p : results.lips_refined_region) {
                transform_point(p, cos_angle, sin_angle, normalized_rect);
            }

            for (auto& p : results.left_eye_refined_region) {
                transform_point(p, cos_angle, sin_angle, normalized_rect);
            }

            for (auto& p : results.right_eye_refined_region) {
                transform_point(p, cos_angle, sin_angle, normalized_rect);
            }

            for (auto& p : results.left_iris_refined_region) {
                transform_point(p, cos_angle, sin_angle, normalized_rect);
            }

            for (auto& p : results.right_iris_refined_region) {
                transform_point(p, cos_angle, sin_angle, normalized_rect);
            }
        }

        //from the refined landmarks, generated the RotatedRect to return.
        {
            float x_min = std::numeric_limits<float>::max();
            float x_max = std::numeric_limits<float>::min();
            float y_min = std::numeric_limits<float>::max();
            float y_max = std::numeric_limits<float>::min();

            for (const auto &p : results.refined_landmarks) {
                x_min = std::min(x_min, p.x);
                x_max = std::max(x_max, p.x);
                y_min = std::min(y_min, p.y);
                y_max = std::max(y_max, p.y);
            }

            float bbox_x = x_min;
            float bbox_y = y_min;
            const float bbox_width = x_max - x_min;
            const float bbox_height = y_max - y_min;

            results.roi.center_x = bbox_x + bbox_width * 0.5f;
            results.roi.center_y = bbox_y + bbox_height * 0.5f;
            results.roi.width = bbox_width;
            results.roi.height = bbox_height;

            //calculate rotation from keypoints 33 & 263
            const float x0 = results.refined_landmarks[33].x * (float)image_width;
            const float y0 = results.refined_landmarks[33].y * (float)image_height;
            const float x1 = results.refined_landmarks[263].x * (float)image_width;
            const float y1 = results.refined_landmarks[263].y * (float)image_height;

            results.roi.rotation = NormalizeRadians(-std::atan2(-(y1 - y0), x1 - x0));

            //final transform
            {
                const float image_width_f = (float)image_width;
                const float image_height_f = (float)image_height;

                float width = results.roi.width;
                float height = results.roi.height;
                const float rotation = results.roi.rotation;

                const float shift_x = 0.f;
                const float shift_y = 0.f;
                const float scale_x = 1.5f;
                const float scale_y = 1.5f;

                if (rotation == 0.f) {
                    results.roi.center_x = results.roi.center_x + width * shift_x;
                    results.roi.center_y = results.roi.center_y + height * shift_y;
                }
                else {
                    const float x_shift =
                        (image_width_f * width * shift_x * std::cos(rotation) -
                            image_height_f * height * shift_y * std::sin(rotation)) /
                        image_width_f;
                    const float y_shift =
                        (image_width_f * width * shift_x * std::sin(rotation) +
                            image_height_f * height * shift_y * std::cos(rotation)) /
                        image_height_f;

                    results.roi.center_x = results.roi.center_x + x_shift;
                    results.roi.center_y = results.roi.center_y + y_shift;
                }

                const float long_side = std::max(width * image_width_f, height * image_height_f);
                width = long_side / image_width_f;
                height = long_side / image_height_f;

                results.roi.width = width * scale_x;
                results.roi.height = height * scale_y;
            }
        }

        rasterize_face_mesh_uv(
            results.refined_landmarks,
            face_triangles,
            image_width,
            image_height
        );

        results.roi = results.roi;
    }
} //namespace onnxmediapipe

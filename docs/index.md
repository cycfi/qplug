---
title: QPlug Audio Plugin Library
---

## Introduction

QPlug is modern c++ library for building Audio plugins that target various
Audio plugin architectures. Qplug is designed to be easy-to-use and has a
clean, and simple modern c++ API.

## Design and Architecture

QPlug sits on top of the [Elements C++ GUI
library](https://github.com/cycfi/elements) which handles user interface, and
the [Q C++ Audio Digital Signal Processing
library](https://github.com/cycfi/Q) for MIDI and digital signal processing
(DSP). Q provides the standardized realtime audio and MIDI streaming
interfaces.

<img src="{{ site.url }}/qplug/assets/images/qplug-arch.png"
    width="35%"
    alt="qplug architecture" />

1. [Elements](https://github.com/cycfi/elements) is a lightweight,
   fine-grained, resolution independent, modular GUI library. Elements, is
   extremely lightweight… and modular. You compose very fine-grained,
   flyweight “elements” to form deep element hierarchies using a declarative
   interface with heavy emphasis on reuse.

2. [Q](https://github.com/cycfi/Q) is a cross-platform C++ library for Audio
   Digital Signal Processing. The Q DSP Library is designed to be simple and
   elegant, leveraging the power of modern C++ and efficient use of
   functional programming techniques, to simplify complex DSP programming
   tasks without sacrificing readability.

QPlug currently targets AU ([Audio Unit](https://apple.co/2WY3nex)) and
[VST3](https://www.steinberg.net/en/company/technologies/vst3.html) using
[iPlug2](https://github.com/iPlug2/iPlug2) as the backend engine. QPlug is
not a framework. It is a modular, and generic library, with loose coupling
between its layers and modules. It is for that matter that QPlug is
retargetable, with a clean, well isolated API which exposes a not-so-thin
abstraction layer on top of architecture-specific plugin frameworks such as
VST and AU as well as architecture-independent plugin frameworks such as
iPlug. It is possible to use other DSP libraries alongside
[Q](https://github.com/cycfi/Q).

> :pencil2: &nbsp; At one point, QPlug directly targeted Au and VST, before
settling down to use iPlug2 which already supports various plugin
architectures.

## Setup and Installation

WIP...

## <a name="jdeguzman"></a>About the Author

Joel got into electronics and programming in the 80s because almost
everything in music, his first love, is becoming electronic and digital.
Since then, he builds his own guitars, effect boxes and synths. He enjoys
playing distortion-laden rock guitar, composes and produces his own music in
his home studio.

Joel de Guzman is the principal architect and engineer at [Cycfi
Research](https://www.cycfi.com/) and a consultant at [Ciere
Consulting](https://ciere.com/). He is a software engineer specializing in
advanced C++ and an advocate of Open Source. He has authored a number of
highly successful Open Source projects such as
[Boost.Spirit](http://tinyurl.com/ydhotlaf),
[Boost.Phoenix](http://tinyurl.com/y6vkeo5t) and
[Boost.Fusion](http://tinyurl.com/ybn5oq9v). These libraries are all part of
the [Boost Libraries](http://tinyurl.com/jubgged), a well respected,
peer-reviewed, Open Source, collaborative development effort.

-------------------------------------------------------------------------------

*Copyright (c) 2014-2020 Joel de Guzman. All rights reserved.*
*Distributed under the [MIT License](https://opensource.org/licenses/MIT)*

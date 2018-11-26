# JamesDSPManager (Audio Effect Digital Signal Processing library for Android)
GUI is based on Omnirom DSP Manager and able to run in all recent Android rom include Samsung, AOSP, Cyanogenmod. 
This app in order to improve your music experience especially you want realistic bass and more natural clarity.
This app don't work too much around with modifying Android framework.

##### Basic:

1. Compression
2. Bass Boost

   --> 4 order IIR and 1023/4095 order FIR linear phase low pass bass boost
3. Reverberation

   --> GVerb and Progenitor 2
4. 10 Band Hybrid Equalizer (1 low shelf, 8 bands shelves, 1 high shelves) (Generate 3/4th order IIR filter on-the-fly, with relatively flat response)
5. Stereo Widen
6. Auto partitioning Convolver

   --> Support mono / stereo / full stereo(LL, LR, RL, RR)
   
   --> Samples per channels should less than 500000

7. Vacuum tube modelling

## Important
### FAQ
#### 1. Computation datatype?

A: Float32

#### 2. Bass boost filter type?

A: Effect have 2 option to boost low frequency, IIR based is obsoleted, eqaulizer already do the job.

   Linear phase filter type is work on convolution basis. When user change the bass boost parameter, engine will compute new low pass filter impulse response.
   
   4095 order FIR filter should work on all frequency listed on options, 1023 order should work well above 80Hz.

#### 3. What is convolver?

A: Convolver is a effect apply convolution mathematical operation on audio signal, that perfectly apply user desired impulse response on music, it could simulate physical space.

   Effect itself require audio file(.wav/.irs/.flac) to become impulse response source, long impulse(> 800000 samples) will be cutoff, however x86 device will have no restriction.

   For more info: [Convolution](https://en.wikipedia.org/wiki/Convolution) and [Convolution reverb](https://en.wikipedia.org/wiki/Convolution_reverb)

#### 3. What is Analog Modelling?

A: Analog Modelling internal work as a vacuum tube amplifier, was designed by [ZamAudio](https://github.com/zamaudio).
The tube they used to model is 12AX7 double triode. They also provide a final stage of tonestack control, it make sound more rich. However, the major parameters is amplifier preamp, this is how even ordered harmonic come from, but this parameter have been limited at maximum 12.0. Input audio amplitude is decided by user, thus louder volume will generate more harmonics and internal amplifier will clip the audio. All analog circuit was built from real mathematical model.
Original is written in C++, for some reasons I ported it to C implementation.

#### 5. What is Misc folder does?

File reverbHeap is modified Progenitor 2 reverb, memory allocation is using heap not stack, it will be useful when you play around with Visual Studio, because VS have 1Mb stack allocation limit...

#### 6. Why you change the Effect structure?

A: This is the only way to full control all DSP effects...

#### 7. Why libjamesdsp.so so big?

A: Because of fftw3 library linked.

#### 8. Why open source? Any license?

A: Audio effects actually is not hard to implement, I don't think close source is a good idea. Many audio effects is exist in the form of libraries, or even in thesis, everyone can implement it...
   This is published under GPLv2.

#### 9. Can I use your effect code?

A: Yes. It is relatively easy to use some effect in other applications. Convolver, reverb, 12AX7 tube amplifier source code is written in similar style, you can look at the how their function is called, initialised, processed, etc. Most of the effect is written in C, so it is easy to port to other platforms without huge modifications.

#### 10. Why no effect after installed?

A: Check step in release build ReadMe.txt.
   audio_effects.conf is file for system to load effect using known UUID, you can just add
   ```
  jdsp {
    path /system/lib/soundfx/libjamesdsp.so
  }
   ```
   ### under
   ```
   bundle {
    path /system/lib/soundfx/libbundlewrapper.so
  }
   ```
   ### AND
   ```
   jamesdsp {
    library jdsp
    uuid f27317f4-c984-4de6-9a90-545759495bf2
  }
   ```
   ### under
   ```
   effects {
   ```

##### On development:
1. Parameterized Room Convolution

	-->May be I will implement this in my other repository: [Complete C implementation of Room Impulse Response Generator]
	(https://github.com/james34602/RIR-Generator)

##### TODO:
1. More optimization on all component (JAVA, Native)

Now work on AOSP, Cyanogenmod, Samsung on Android 5.0, 6.0 and 7.0/7.1(TESTED)

## Download Link
1. See my project release page

# Screenshot
1. [Equalizer screenshot(Dark theme)](https://rawgit.com/james34602/JamesDSPManager/master/ScreenshotMainApp1.png)
2. [Convolver screenshot(Idea theme)](https://rawgit.com/james34602/JamesDSPManager/master/ScreenshotMainApp2.png)

# Important

We won't modify SELinux, let your device become more safe.
Also, it is good idea to customize your own rom or even port rom.

## How to install?
See readme in download link.

# Contact
Better contact me by email. Send to james34602@gmail.com

# Terms and Conditions / License
The engine frame is based on Antti S. Lankila's DSPManager

All compatibility supporting by James Fung

Android framework components by Google

Advanced IIR filters library by Vinnie Falco, modify by Bernd Porr, with shrinked to minimum features needed.

Source code is provided under the [GPL V2](https://www.gnu.org/licenses/old-licenses/gpl-2.0.html)

### More Credit
DSPFilter.xlsx is a tool for you to desgin IIR Biquad Filter, it is a component from miniDSP.

RBJ_Eq.xls is a RBJ Biquad Equalizer designer. Not very useful and I will not implement it, but could be a reference to designing equalizer.

# Structure map generated by Understand (Hosted on rawgit)
<a><img src="https://rawgit.com/james34602/JamesDSPManager/master/libjamesdsp_StructureMap.svg"/></a>

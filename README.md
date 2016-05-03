# unpacking revisited


This thing contains two video tutorials (and one PIN tool) to aid unpacking some common malware cases, in particular Java and .Net packed malware.

_Unpacking Java and .Net malware_

The two videos below illustrate an idea that came to my mind after having analyzed certain number of Java and .Net samples. The key observation was that those packers always use the same standard library method calls to load dynamically instantiated classes. So I have used JD-Eclipse for Java and SharpDevelop for .Net to place conditional breakpoints on the appropriate standard library methods that load dynamically instantiated classes or assemblies - a constructor of JarInputStream class for Java, and Assembly.Load() method for .Net. The conditional breakpoints should contain code to dump the newly instantiated class, Jar archive or assembly file from a byte array-like object that is passed to them via arguments. Here are the videos and the corresponding conditional breakpoint expressions:

__Java (Eclipse)__

[![Java unpacking](https://i.vimeocdn.com/video/568901375_960.webp)](https://vimeo.com/165124535)

Conditional BP code:
```
int size = arg0.available();
byte[] b = new byte[size];
arg0.read(b, 0, size);
arg0.reset();
FileOutputStream fos = new FileOutputStream(new File("C:/newjar.jar"));
fos.write(b);
fos.close();
return true;
```

__.Net (SharpDevelop)__

[![.Net unpacking](https://i.vimeocdn.com/video/567270389_960.webp)](https://vimeo.com/163747528)

Conditional BP code:
```
File.WriteAllBytes("c:\newass.exe", rawAssembly)
```
Note: SharpDevelop will complain a bit when the conditional breakpoint expression gets evaluated but it can safely be ignored.

The two tutorials are based on real malware samples that are pretty common, for Java it is JSocket/Adwind variant (7b4dcd69728520f865faeb46b1dbabe7) and for .NET - Kazy variant (4fad68772b12eac7f03169c6058087e1). Note that some malware can do it via other library method calls. For example, in Java you can directly invoke ClassLoader.defineClass() to instantiate a single class instead of a full JAR file. In this case you need to place a breakpoint on the corresponding method instead.

_Generic unpacker for native code_

As a bonus, here is a very simple PIN-based unpacker that was a result of my experiments with Intel PIN. It is based on http://joxeankoret.com/blog/2012/11/04/a-simple-pin-tool-unpacker-for-the-linux-version-of-skype/ so please read this blog post first. In my case though I wanted to also cover the code that is inserted in existing memory segments and not only newly allocated segments. So I am keeping a bitmap of every single write into memory and then checking if they are executed later on. The change I decided to make to the code from the above blog post is that instead of C++ deques I moved to vectors (as they are way faster). Please note that this PIN tool needs a lot of memory so be generous in giving memory to your VM on which you will use it. Also, it is quite CPU-intensive so give it some time. My recommendation is also to dedicate two (or more) virtual processors to the VM. Don't forget to run PIN with a regular debugger connected (as described in the mentioned blog post); the debugger will receive a breakpoint when the execution of modified/written memory occurs. Oh btw, sorry but there is no video for this one.

If you have some ideas or comments please do not hesitate to contact me - I am waiting for this !



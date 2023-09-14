// From Full Moon info site. (but only had it there temporarily, to record the trailer)
    
    //XXX Testing video capture
    // ha ha ha oh my god this is so much easier than finding a native screencap tool that actually works
    this.dom.spawn(flexor, "INPUT", { type: "button", value: "Capture", "on-click": () => this.onCapture() });
  }
  
  //XXX
  onCapture() {
    console.log(`PlayUi.onCapture`, { recorder: this.recorder });
    if (this.recorder) {
      console.log(`stopping record`);
      this.recorder.stop();
      this.recorder = null;
      return;
    }
    const options = {
      video: true,
      audio: true,
      //preferCurrentTab: true,
      //selfBrowserSurface: 'include',
    };
    navigator.mediaDevices.getDisplayMedia(options)
      .then(result => {
        console.log(`getDisplayMedia ok`, result);
        const recorder = new MediaRecorder(result);
        console.log(`MediaRecorder`, recorder);
        const chunks = [];
        recorder.addEventListener("dataavailable", (chunk) => {
          chunks.push(chunk.data);
        });
        recorder.addEventListener("stop", () => {
          console.log(`recorder stopped. let's encode and save it`, recorder);
          const blob = new Blob(chunks, { type: "video/mp4" });
          console.log(`created blob`, blob);
          const url = URL.createObjectURL(blob);
          console.log(`url: ${url}`);
          window.open(url, "_blank");
        });
        console.log(`delaying capture start by 5 seconds -- switch to fullscreen now!`);
        window.setTimeout(() => {
          console.log(`...begin recording`);
          recorder.start();
        }, 5000);
        this.recorder = recorder;
      }).catch(error => {
        console.log(`getDisplayMedia failed`, error);
      });
  }

// FSK Modulation
// Rob Dobson 2018
// Loosely based on https://github.com/sixteenmillimeter/Javascript-FSK-Serial-Generator-for-Mobile-Safari

class FSKMod {
    constructor(sampleRate, symbolRate, freqHigh, freqLow) {
        this.audioContext = null;
        this.audioSource = null;
        this.sampleRate = sampleRate;
        this.symbolRate = symbolRate;
        this.freqHigh = freqHigh;
        this.freqLow = freqLow;
    }

    async generate(bitStreamArray) {
        // Check len & UTF-8
        if (bitStreamArray.length === 0)
            return;

        // Modulation settings
        const samplesPerSymbol = this.sampleRate / this.symbolRate;
        const preambleSymbols = 20;
        const postambleSymbols = 5;
        const size = (preambleSymbols + postambleSymbols + bitStreamArray.length) * samplesPerSymbol;

        let data = "RIFF" + this.chr32(size + 36) + "WAVE" +
            "fmt " + this.chr32(16, 0x00010001, this.sampleRate, this.sampleRate, 0x00080001) +
            "data" + this.chr32(size);

        const genManchesterData = function (symbolValue, samplesPerSymbol, that) {
            let genData = ""
            for (let i = 0; i < samplesPerSymbol; i++) {

                let v = 0;
                if (i < samplesPerSymbol / 2)
                    v = 128 + 127 * Math.sin((2 * Math.PI) * (i / that.sampleRate) * (symbolValue ? that.freqHigh : that.freqLow));
                else
                    v = 128 + 127 * Math.sin((2 * Math.PI) * (i / that.sampleRate) * (symbolValue ? that.freqLow : that.freqHigh));
                genData += that.chr8(v);
            }
            return genData
        };

        // Preamble is alternating 0 and 1 symbols
        for (let i = 0; i < preambleSymbols; i++)
            data += genManchesterData((i % 2), samplesPerSymbol, this);

        // Datastream from bits
        for (let chIdx = 0; chIdx < bitStreamArray.length; chIdx++) {
            let c = bitStreamArray[chIdx];
            data += genManchesterData(c & 1, samplesPerSymbol, this);
        }

        // Postamble is all 0
        for (let i = 0; i < postambleSymbols; i++)
            data += genManchesterData(0, samplesPerSymbol, this);

        // Check size is correct
        if (size + 44 !== data.length)
            alert("WAV data is wrong size: " + size + 44 + " != " + data.length);

        // Encode the data
        let dataURI = encodeURI(btoa(data));

        // And decode into a buffer
        const arrayBuff = Base64Binary.decodeArrayBuffer(dataURI);
        this.getAudioContext();
        return await this.audioContext.decodeAudioData(arrayBuff);
    }

    play(audioData) {
        this.getAudioContext();
        this.audioSource = this.audioContext.createBufferSource();
        this.audioSource.buffer = audioData;
        this.audioSource.connect(this.audioContext.destination);
        if ('AudioContext' in window) {
            this.audioSource.start(0);
        } else if ('webkitAudioContext' in window) {
            this.audioSource.noteOn(0);
        }
    }

    getAudioContext()
    {
        if (this.audioContext === null) {
            if ('AudioContext' in window) {
                this.audioContext = new AudioContext();
            } else if ('webkitAudioContext' in window) {
                this.audioContext = new webkitAudioContext();
            } else {
                alert('WebAudio not supported');
            }
        }
        return this.audioContext;
    }

    chr8() {
        return Array.prototype.map.call(arguments, function (a) {
            return String.fromCharCode(a & 0xff)
        }).join('');
    }
    chr16() {
        return Array.prototype.map.call(arguments, function (a) {
            return String.fromCharCode(a & 0xff, (a >> 8) & 0xff)
        }).join('');
    }
    chr32() {
        return Array.prototype.map.call(arguments, function (a) {
            return String.fromCharCode(a & 0xff, (a >> 8) & 0xff, (a >> 16) & 0xff, (a >> 24) & 0xff);
        }).join('');
    }

}

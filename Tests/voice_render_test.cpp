// Decu6000 - Copyright (C) 2026 LexLongcoding
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "../Source/KickVoice.h"
#include "../Source/SnareVoice.h"
#include "../Source/HatVoice.h"
#include "../Source/TomVoice.h"
#include "../Source/CymbalVoice.h"
#include "../Source/PercVoice.h"
#include "../Source/CrystalVoice.h"
#include "../Source/LiquidVoice.h"
#include "../Source/BreathVoice.h"
#include "../Source/DigitalVoice.h"
#include "../Source/BurstVoice.h"
#include <cstdio>
#include <vector>
#include <string>
#include <cstring>

static void writeWav(const std::string& path, const std::vector<float>& x, int sr){
    FILE* f=fopen(path.c_str(),"wb"); int n=(int)x.size(); int bytes=n*2;
    auto w32=[&](uint32_t v){fwrite(&v,4,1,f);}; auto w16=[&](uint16_t v){fwrite(&v,2,1,f);};
    fwrite("RIFF",1,4,f); w32(36u+(uint32_t)bytes); fwrite("WAVEfmt ",1,8,f); w32(16); w16(1); w16(1);
    w32((uint32_t)sr); w32((uint32_t)sr*2); w16(2); w16(16); fwrite("data",1,4,f); w32((uint32_t)bytes);
    for(float s: x){ float c=std::clamp(s,-1.f,1.f); int16_t v=(int16_t)std::lround(c*32767.f); fwrite(&v,2,1,f);} fclose(f);
}
struct Ev{int pos; int voice; float vel;};
int main(int argc,char**argv){
    int sr = argc>1? atoi(argv[1]) : 48000;
    KickVoice kick; SnareVoice snare; HatVoice ch{1u}, oh{2u};
    TomVoice lt{11u}, mt{12u}, ht{13u}; CymbalVoice crash{21u}, ride{22u};
    kick.prepare(sr); snare.prepare(sr); ch.prepare(sr); oh.prepare(sr);
    lt.prepare(sr); mt.prepare(sr); ht.prepare(sr); crash.prepare(sr); ride.prepare(sr);
    TomVoice::Params tp, ltp, htp; ltp.tuneHz=82.f; htp.tuneHz=184.f; tp.tuneHz=123.f;
    lt.setParams(ltp); mt.setParams(tp); ht.setParams(htp);
    CymbalVoice::Params cp; crash.setParams(cp);
    CymbalVoice::Params rp; rp.tune=0.62f; rp.decaySec=3.f; rp.level=0.45f; rp.tone=0.75f; rp.ring=0.9f; ride.setParams(rp);
    SnareVoice::Params sp; HatVoice::Params hp; KickVoice::Params kp;
    snare.setParams(sp); HatVoice::Params chp=hp; chp.decaySec=0.06f; ch.setParams(chp);
    HatVoice::Params ohp=hp; ohp.decaySec=0.6f; oh.setParams(ohp); kick.setParams(kp);

    // individual hits
    auto solo=[&](DrumVoice& v,const char* name,float secs){
        std::vector<float> out((size_t)(secs*sr)); v.trigger(1.f);
        size_t last=0; float peak=0; bool nan=false;
        for(size_t i=0;i<out.size();++i){ out[i]=v.process(); if(!std::isfinite(out[i])) nan=true;
            peak=std::max(peak,std::abs(out[i])); if(v.isActive()) last=i; }
        printf("%-10s peak=%.3f active_for=%.3fs nan=%d\n",name,peak,(double)last/sr,nan);
        writeWav(std::string(name)+".wav",out,sr);
    };
    solo(snare,"snare",1.5f); solo(ch,"chat",1.0f); solo(oh,"ohat",2.0f);
    solo(lt,"lotom",2.5f); solo(mt,"midtom",2.5f); solo(ht,"hitom",2.5f); solo(crash,"crash",6.0f); solo(ride,"ride",8.0f);
    for(float b: {0.f, 1.f}){ TomVoice::Params q=tp; q.bend=b; q.decaySec=2.0f; mt.setParams(q);
        solo(mt, b==0.f? "tom_bend0":"tom_bend1", 3.0f); }
    { TomVoice::Params q=htp; q.tuneHz=360.f; q.bend=1.f; q.drive=1.f; q.crush=1.f; q.noise=1.f; q.decaySec=0.05f; ht.setParams(q); solo(ht,"tom_extreme",1.f); ht.setParams(htp); }
    { CymbalVoice::Params q=cp; q.tune=1.f; q.tone=1.f; q.crush=1.f; q.decaySec=5.f; crash.setParams(q); solo(crash,"crash_extreme",7.f); crash.setParams(cp); }
    mt.setParams(tp);
    // FM percussion: Perc 1 (woody) and Perc 2 (metallic) defaults, plus extremes
    PercVoice p1, p2; p1.prepare(sr); p2.prepare(sr);
    PercVoice::Params pp1; pp1.pitchHz=720.f; pp1.decaySec=0.07f; pp1.ratio=2.0f; pp1.index=0.25f; pp1.level=0.7f;
    PercVoice::Params pp2; pp2.pitchHz=540.f; pp2.decaySec=0.3f; pp2.ratio=1.41f; pp2.index=0.55f; pp2.level=0.55f;
    p1.setParams(pp1); p2.setParams(pp2);
    solo(p1,"perc1",1.f); solo(p2,"perc2",1.5f);
    { PercVoice::Params q=pp2; q.pitchHz=2000.f; q.ratio=8.f; q.index=1.f; q.crush=1.f; q.decaySec=1.5f; p2.setParams(q); solo(p2,"perc_extreme",2.f);
      q=pp1; q.pitchHz=100.f; q.ratio=0.5f; q.index=0.f; p2.setParams(q); solo(p2,"perc_low",1.f); p2.setParams(pp2); }
    // bio-neural: Crystal Impact and Liquid Metal, defaults + settings sweeps
    CrystalVoice cr; LiquidVoice lq; cr.prepare(sr); lq.prepare(sr);
    CrystalVoice::Params crp; LiquidVoice::Params lqp; cr.setParams(crp); lq.setParams(lqp);
    solo(cr,"crystal",5.f); solo(lq,"liquid",3.f);
    { auto q=crp; q.inharm=0.f; q.shimmer=0.f; q.pitchHz=440.f; cr.setParams(q); solo(cr,"crystal_harmonic",4.f);
      q.inharm=1.f; cr.setParams(q); solo(cr,"crystal_bar",4.f);
      q=crp; q.pitchHz=4000.f; q.shimmer=1.f; q.decaySec=6.f; cr.setParams(q); solo(cr,"crystal_extreme",8.f);
      q=crp; q.pitchHz=200.f; cr.setParams(q); cr.trigger(0.2f); solo(cr,"crystal_soft_low",5.f); cr.setParams(crp); }
    { auto q=lqp; q.drip=1.f; q.modRateHz=3.f; q.decaySec=2.f; lq.setParams(q); solo(lq,"liquid_drip",3.f);
      q.drip=0.f; lq.setParams(q); solo(lq,"liquid_nodrip",3.f);
      q=lqp; q.pitchHz=2000.f; q.modRateHz=30.f; q.drip=1.f; q.decaySec=4.f; lq.setParams(q); solo(lq,"liquid_extreme",5.f);
      lq.setParams(lqp); }
    // bio-neural: Synthetic Breath, Digital Perc, Bio Burst
    BreathVoice br; DigitalVoice dg; BurstVoice bu; br.prepare(sr); dg.prepare(sr); bu.prepare(sr);
    BreathVoice::Params brp; DigitalVoice::Params dgp; BurstVoice::Params bup;
    br.setParams(brp); dg.setParams(dgp); bu.setParams(bup);
    solo(br,"breath",1.f); solo(dg,"digital",1.f); solo(bu,"burst",2.f);
    { auto q=brp; q.breath=0.f; q.decaySec=1.f;
      for (float v : {0.f, 0.5f, 1.f}) { q.vowel=v; br.setParams(q); solo(br, v==0.f?"breath_U":(v==0.5f?"breath_A":"breath_I"), 1.2f); }
      q.breath=1.f; q.vowel=0.5f; br.setParams(q); solo(br,"breath_whisper",1.2f);
      q=brp; q.pitchHz=400.f; q.vowel=1.f; q.decaySec=2.f; br.setParams(q); solo(br,"breath_extreme",2.5f); br.setParams(brp); }
    { auto q=dgp; q.chaos=0.f; q.fold=0.f; q.decaySec=0.5f; dg.setParams(q); solo(dg,"digital_clean",1.f);
      q.fold=1.f; dg.setParams(q); solo(dg,"digital_fold",1.f);
      q.fold=0.f; q.chaos=1.f; dg.setParams(q); solo(dg,"digital_chaos",1.f);
      q=dgp; q.pitchHz=2000.f; q.chaos=1.f; q.fold=1.f; q.decaySec=1.5f; dg.setParams(q); solo(dg,"digital_extreme",2.f); dg.setParams(dgp); }
    { auto q=bup; q.swarm=0.f; q.grain=0.f; bu.setParams(q); solo(bu,"burst_sparse",1.5f);
      q.swarm=1.f; q.grain=1.f; bu.setParams(q); solo(bu,"burst_dense",1.5f);
      q=bup; q.pitchHz=6000.f; q.swarm=1.f; q.grain=0.f; q.decaySec=3.f; bu.setParams(q); solo(bu,"burst_extreme",4.f);
      q.pitchHz=150.f; q.grain=1.f; bu.setParams(q); solo(bu,"burst_low",4.f); bu.setParams(bup); }
    // crushed snare
    sp.crush=0.7f; sp.drive=0.6f; snare.setParams(sp); solo(snare,"snare_crush",1.0f);
    sp=SnareVoice::Params{}; snare.setParams(sp);

    // choke test: open hat at 0, closed hat at 0.15s
    { std::vector<float> o((size_t)sr); oh.trigger(1.f); int cpos=(int)(0.15*sr);
      size_t ohEnd=0;
      for(int i=0;i<sr;++i){ if(i==cpos){ oh.choke(); ch.trigger(1.f);} float a=oh.process(); if(oh.isActive()) ohEnd=(size_t)i; o[(size_t)i]=a+ch.process(); }
      printf("choke: open hat stopped %.1f ms after choke\n", 1000.0*((double)ohEnd-cpos)/sr);
      writeWav("choke.wav",o,sr); }

    // beat: trip-hop-ish 90 bpm, 2 bars
    float bpm=90; int step=(int)(sr*60.0/bpm/4); int bars=2;
    std::vector<float> beat((size_t)(step*16*bars + 3*sr));
    const char* K="x.....x...x.....", *S="....x.......x...", *C="x.x.x.x.x.x.x.x.", *O="..............x.";
    // bar 2 ends in a tom fill; the crash lands on the downbeat after the last bar
    const char* P1="..x.......x..x..", *P2=".......x......x.";
    const char* L="..............xx", *M="............xx..", *H="..........xx....";
    float peak=0;
    for(size_t i=0;i<beat.size();++i){
        int idx=(int)(i/(size_t)step), off=(int)(i%(size_t)step);
        if(off==0 && idx<16*bars){ int s=idx%16;
            if(K[s]=='x') kick.trigger(1.f); if(S[s]=='x') snare.trigger(0.9f);
            bool fill = idx/16 == bars-1;
            if(!fill || s<10){ if(C[s]=='x'){ oh.choke(); ch.trigger(s%4==0?0.8f:0.5f);} if(O[s]=='x') oh.trigger(0.7f); }
            if(!fill){ if(P1[s]=='x') p1.trigger(0.7f); if(P2[s]=='x') p2.trigger(0.6f); }
            if(fill){ if(L[s]=='x') lt.trigger(0.9f); if(M[s]=='x') mt.trigger(0.9f); if(H[s]=='x') ht.trigger(0.9f); } }
        if(i==(size_t)(step*16*bars)){ kick.trigger(1.f); crash.trigger(1.f); }
        float v=(kick.process()+snare.process()+ch.process()+oh.process()
                 +lt.process()+mt.process()+ht.process()+crash.process()+p1.process()+p2.process())*0.8f;
        v=Dsp::softLimit(v); peak=std::max(peak,std::abs(v)); beat[i]=v;
    }
    printf("beat peak=%.3f\n",peak); writeWav("beat.wav",beat,sr);
}

// Web UI served at "/" (raw string literal, stored in flash)
#pragma once

static const char INDEX_HTML[] PROGMEM = R"html(<!DOCTYPE html><html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1,viewport-fit=cover"><meta name="theme-color" content="#0B0F23">
<title>Stair Light</title>
<link rel="preconnect" href="https://fonts.googleapis.com">
<link rel="stylesheet" href="https://fonts.googleapis.com/css2?family=Unbounded:wght@500;700&family=Manrope:wght@400;500;600;700&display=swap" media="print" onload="this.media='all'">
<style>
@property --glow { syntax: '<color>'; inherits: true; initial-value: #7C8CFF; }
:root{
  color-scheme: dark;
  --night:#0B0F23; --deep:#12173A; --panel:rgba(27,33,72,.58); --panel-hi:rgba(40,48,100,.55);
  --line:rgba(150,162,232,.16); --line-hi:rgba(170,180,250,.32);
  --text:#EEF0FC; --muted:#9BA1C8; --dim:#62689A;
  --glow:#7C8CFF;
  --ok:#4FE0A0; --warn:#FFB65C; --bad:#FF5E7E;
  --display:"Unbounded","Arial Black",system-ui,sans-serif;
  --body:"Manrope",ui-sans-serif,system-ui,-apple-system,"Segoe UI",Roboto,sans-serif;
  transition: --glow .8s ease;
}
*{box-sizing:border-box}
[hidden]{display:none!important}
html{background:var(--night)}
html{-webkit-text-size-adjust:100%;text-size-adjust:100%}
body{margin:0;background:var(--night);color:var(--text);font:15px/1.5 var(--body);-webkit-font-smoothing:antialiased}
body::before{content:"";position:fixed;inset:0;z-index:0;pointer-events:none;
  background:
    radial-gradient(70% 45% at 78% 4%, color-mix(in srgb, var(--glow) 34%, transparent), transparent 70%),
    radial-gradient(55% 40% at 6% 34%, rgba(88,52,190,.22), transparent 72%),
    radial-gradient(60% 40% at 50% 104%, color-mix(in srgb, var(--glow) 16%, transparent), transparent 70%);}
.app{position:relative;z-index:1;max-width:480px;margin:0 auto;padding-inline:16px;padding-block:20px calc(32px + env(safe-area-inset-bottom,0px));display:flex;flex-direction:column;gap:16px}
button,input,select{font:inherit;color:inherit}
button{cursor:pointer}
:focus-visible{outline:2px solid var(--glow);outline-offset:2px}

/* header */
.top{display:flex;justify-content:space-between;align-items:flex-end;gap:12px}
.wordmark{font-family:var(--display);font-weight:700;font-size:26px;line-height:1;letter-spacing:-.01em;
  background:linear-gradient(100deg,#fff 30%, var(--glow) 100%);-webkit-background-clip:text;background-clip:text;color:transparent}
.host{color:var(--muted);font-size:13px;margin-top:6px}
.host b{color:var(--text);font-weight:600}
.clock{text-align:right;font-variant-numeric:tabular-nums}
.clock .time{font-family:var(--display);font-weight:500;font-size:22px;line-height:1}
.clock .date{color:var(--muted);font-size:13px;margin-top:6px}

/* notices under the header (night mode, connection) */
.notices{display:flex;flex-wrap:wrap;gap:8px}
.notices:not(:has(.chip:not([hidden]))){display:none}
.chip{display:inline-flex;align-items:center;gap:7px;padding:6px 11px;border-radius:999px;font-size:12.5px;font-weight:600;
  background:rgba(255,255,255,.06);border:1px solid var(--line);color:var(--muted)}
.chip i{width:7px;height:7px;border-radius:50%;background:var(--dim)}
.chip.night i{background:#ff4d4d;box-shadow:0 0 8px #ff4d4d}

/* panels */
.panel{background:var(--panel);border:1px solid var(--line);border-radius:20px;padding:16px;
  backdrop-filter:blur(14px);-webkit-backdrop-filter:blur(14px)}
.panel h2{font-family:var(--display);font-weight:500;font-size:15px;margin:0;letter-spacing:.01em}
.panel .head{display:flex;justify-content:space-between;align-items:baseline;gap:10px;margin-bottom:12px}
.panel .hint{color:var(--muted);font-size:12.5px}

/* automation */
.auto{display:flex;align-items:center;gap:14px}
.auto .txt{flex:1}
.auto .title{font-weight:700;font-size:16px}
.auto .desc{color:var(--muted);font-size:13px}
.switch{position:relative;width:62px;height:36px;border-radius:999px;border:1px solid var(--line-hi);background:rgba(255,255,255,.07);flex-shrink:0;transition:background .25s}
.switch::after{content:"";position:absolute;top:3px;left:3px;width:28px;height:28px;border-radius:50%;background:#C9CDEA;transition:transform .25s, background .25s, box-shadow .25s}
.switch[aria-checked="true"]{background:color-mix(in srgb, var(--ok) 30%, transparent);border-color:color-mix(in srgb, var(--ok) 60%, transparent)}
.switch[aria-checked="true"]::after{transform:translateX(26px);background:#fff;box-shadow:0 0 14px var(--ok)}

/* scenes */
.scenes{display:grid;grid-template-columns:repeat(3,1fr);gap:10px}
.scene{position:relative;display:flex;flex-direction:column;gap:8px;padding:8px 8px 10px;border-radius:16px;border:1px solid var(--line);
  background:rgba(255,255,255,.035);text-align:left;transition:border-color .2s, transform .15s, box-shadow .2s}
.scene:active{transform:scale(.97)}
.scene .sw{height:58px;border-radius:11px;position:relative;overflow:hidden}
.scene .nm{font-weight:700;font-size:13px;line-height:1.2;padding-inline:2px}
.scene.playing{border-color:color-mix(in srgb, var(--glow) 70%, transparent);box-shadow:0 0 0 1px color-mix(in srgb, var(--glow) 50%, transparent),0 10px 30px -10px var(--glow)}
.scene .ring{position:absolute;right:12px;top:12px;width:20px;height:20px;border-radius:50%;
  background:conic-gradient(#fff calc(var(--p,0)*1%), rgba(255,255,255,.18) 0);
  -webkit-mask:radial-gradient(circle 6px, transparent 98%, #000 100%);mask:radial-gradient(circle 6px, transparent 98%, #000 100%)}
.sw-rainbow{background:linear-gradient(90deg,#ff3b5c,#ffae3b,#f2ff3b,#3bff8a,#3bd2ff,#6a3bff,#ff3bd0,#ff3b5c);background-size:200% 100%;animation:flow 3.5s linear infinite}
.sw-white{background:radial-gradient(120% 90% at 50% 120%, #fff7ea 0%, #ffd9a6 45%, #5b4a35 100%)}
.sw-stars{background:
  radial-gradient(1.6px 1.6px at 18% 30%, #fff 60%, transparent),
  radial-gradient(1.2px 1.2px at 64% 22%, #fff 60%, transparent),
  radial-gradient(2px 2px at 80% 64%, #fff 60%, transparent),
  radial-gradient(1.3px 1.3px at 38% 72%, #cfe0ff 60%, transparent),
  radial-gradient(1.6px 1.6px at 52% 48%, #fff 60%, transparent),
  linear-gradient(160deg,#16205e,#0b1240);animation:twinkle 2.4s ease-in-out infinite}
.sw-bday{background:
  radial-gradient(3px 3px at 14% 28%, #ff4f7b 70%, transparent),
  radial-gradient(3px 3px at 30% 66%, #4fe0ff 70%, transparent),
  radial-gradient(3px 3px at 46% 34%, #ffe14f 70%, transparent),
  radial-gradient(3px 3px at 62% 74%, #8a5bff 70%, transparent),
  radial-gradient(3px 3px at 78% 30%, #4fff9b 70%, transparent),
  radial-gradient(3px 3px at 88% 70%, #ff9a3d 70%, transparent),
  radial-gradient(3px 3px at 22% 84%, #ffe14f 70%, transparent),
  linear-gradient(160deg,#2a1540,#130b28);animation:twinkle 3s ease-in-out infinite reverse}
.sw-night{background:radial-gradient(90% 120% at 50% 110%, #ff2a2a 0%, #6d0d12 48%, #1c0507 100%);animation:breathe 4s ease-in-out infinite}
.sw-matrix{background:#010a03}
.sw-matrix .mx{position:absolute;top:0;width:1ch;font:700 10px/10px ui-monospace,Menlo,Consolas,monospace;word-break:break-all;text-align:center;
  background:linear-gradient(180deg,transparent 0%,rgba(30,255,100,.25) 30%,#1eff64 88%,#f2fff5 100%);-webkit-background-clip:text;background-clip:text;color:transparent;
  filter:drop-shadow(0 0 2px rgba(30,255,100,.8));animation:mxfall linear infinite;transform:translateY(-100%)}
@keyframes mxfall{to{transform:translateY(72px)}}
@keyframes flow{to{background-position:-200% 0}}
@keyframes twinkle{50%{filter:brightness(1.45)}}
@keyframes breathe{50%{filter:brightness(.62)}}

/* colour */
.swatch-bar{height:44px;border-radius:12px;margin-bottom:14px;border:1px solid var(--line);
  background:linear-gradient(90deg, var(--mix,#111) 0%, var(--mix,#111) 100%);box-shadow:0 12px 30px -14px var(--mix,transparent)}
.ch{display:grid;grid-template-columns:22px 1fr 46px;align-items:center;gap:10px;margin:10px 0}
.ch label{font-weight:700;font-size:13px;color:var(--muted)}
.ch output{text-align:right;font-variant-numeric:tabular-nums;font-weight:600;font-size:13px}
input[type=range]{-webkit-appearance:none;appearance:none;width:100%;height:10px;border-radius:999px;background:var(--track);outline:none}
input[type=range]::-webkit-slider-thumb{-webkit-appearance:none;width:24px;height:24px;border-radius:50%;background:#fff;border:3px solid var(--thumb);box-shadow:0 0 14px var(--thumb)}
input[type=range]::-moz-range-thumb{width:20px;height:20px;border-radius:50%;background:#fff;border:3px solid var(--thumb);box-shadow:0 0 14px var(--thumb)}
#slR{--track:linear-gradient(90deg,#2a1420,#ff2d4a);--thumb:#ff2d4a}
#slG{--track:linear-gradient(90deg,#10261c,#2dff8a);--thumb:#2dff8a}
#slB{--track:linear-gradient(90deg,#121a36,#2d7dff);--thumb:#2d7dff}
#slW{--track:linear-gradient(90deg,#26221c,#fff1d6);--thumb:#fff1d6}
.presets{display:flex;gap:8px;flex-wrap:wrap;margin-top:12px;align-items:center}
.presets span{color:var(--muted);font-size:13px;margin-right:2px}
.pill{border:1px solid var(--line-hi);background:rgba(255,255,255,.05);padding:7px 13px;border-radius:999px;font-weight:600;font-size:13px;font-variant-numeric:tabular-nums}
.pill:hover{background:rgba(255,255,255,.1)}
.note{margin-top:12px;font-size:12.5px;color:var(--warn)}

/* motion log */
.log{list-style:none;margin:0;padding:0;display:flex;flex-direction:column}
.log li{display:grid;grid-template-columns:30px 1fr auto;align-items:center;gap:10px;padding:9px 0;border-top:1px solid var(--line)}
.log li:first-child{border-top:0;padding-top:2px}
.dir{width:30px;height:30px;border-radius:10px;display:grid;place-items:center;background:rgba(255,255,255,.06)}
.dir svg{width:16px;height:16px;stroke:var(--text)}
.log .what{font-weight:600;font-size:14px}
.log .sub{display:flex;align-items:center;gap:6px;color:var(--muted);font-size:12.5px}
.dot{width:9px;height:9px;border-radius:50%;flex-shrink:0}
.log li.empty{display:block;color:var(--muted);font-size:13px;border-top:0}
.log time{color:var(--muted);font-variant-numeric:tabular-nums;font-size:13px}

/* device */
.stats{display:grid;grid-template-columns:1fr 1fr;gap:10px}
.stat{padding:12px;border-radius:14px;background:rgba(255,255,255,.04);border:1px solid var(--line)}
.stat .k{color:var(--muted);font-size:12.5px}
.stat .v{font-weight:700;font-size:16px;font-variant-numeric:tabular-nums;margin-top:2px}
.bar{height:6px;border-radius:3px;background:rgba(255,255,255,.08);margin-top:8px;overflow:hidden}
.bar i{display:block;height:100%;border-radius:3px;background:linear-gradient(90deg,var(--ok),#b7f5d8)}
.bars{display:flex;gap:3px;align-items:flex-end;height:16px;margin-top:6px}
.bars i{width:5px;border-radius:2px;background:rgba(255,255,255,.15)}
.bars i.on{background:var(--ok)}
.kv{width:100%;border-collapse:collapse;font-size:13px;margin-top:8px;font-variant-numeric:tabular-nums}
.kv td{padding:6px 0;border-top:1px solid var(--line)}
.kv td:first-child{color:var(--muted);width:42%}

/* collapsible sections */
details.sect > summary{list-style:none;cursor:pointer;display:flex;justify-content:space-between;align-items:center}
details.sect > summary::-webkit-details-marker{display:none}
details.sect > summary .chev{width:28px;height:28px;border-radius:9px;display:grid;place-items:center;background:rgba(255,255,255,.06);transition:transform .2s}
details.sect[open] > summary .chev{transform:rotate(180deg)}
details.sect .body{margin-top:14px;display:flex;flex-direction:column;gap:12px}
.field{display:flex;flex-direction:column;gap:6px}
.field label{font-size:13px;color:var(--muted);font-weight:600}
.inp{font-size:16px;background:rgba(5,8,24,.55);border:1px solid var(--line-hi);border-radius:11px;padding:10px 12px;min-width:0}
.row{display:flex;gap:10px;align-items:center;flex-wrap:wrap}
.btn{border:0;border-radius:12px;padding:11px 16px;font-weight:700;font-size:14px;background:linear-gradient(135deg,var(--glow),#b9a6ff);color:#0B0F23}
.btn.ghost{background:rgba(255,255,255,.07);color:var(--text);border:1px solid var(--line-hi)}
.btn.danger{background:rgba(255,94,126,.14);color:#ffb3c2;border:1px solid rgba(255,94,126,.4)}
.status{font-size:13px;color:var(--ok)}
.bday{display:grid;grid-template-columns:62px 62px 1fr 38px;gap:8px;align-items:center}
.bday .x{height:40px;border-radius:11px;border:1px solid var(--line);background:rgba(255,255,255,.05);color:var(--muted)}
.mini{font-size:12.5px;color:var(--dim)}
.confirm{display:flex;gap:10px;align-items:center;flex-wrap:wrap;padding:12px;border-radius:14px;background:rgba(255,94,126,.08);border:1px solid rgba(255,94,126,.3)}
footer{color:var(--dim);font-size:12.5px;text-align:center;padding-top:6px}

@media (prefers-reduced-motion: reduce){
  .sw-rainbow,.sw-stars,.sw-bday,.sw-night,.sw-matrix .mx{animation:none}
  .sw-matrix .mx{transform:none}
  :root{transition:none}
}
@media (max-width:360px){ .scenes{grid-template-columns:repeat(2,1fr)} }
</style>
</head><body>
<div class="app">
  <header class="top">
    <div>
      <div class="wordmark">Stair Light</div>
      <div class="host"><b id="hostName">stairlight</b> on <span id="ssid">WiFi</span></div>
    </div>
    <div class="clock"><div class="time" id="clockTime">--:--</div><div class="date" id="clockDate">&nbsp;</div></div>
  </header>

  <div class="notices">
    <span class="chip night" id="chipNight" hidden><i></i><span>Night mode</span></span>
    <span class="chip" id="chipConn" hidden><i style="background:var(--bad);box-shadow:0 0 8px var(--bad)"></i><span>Not connected, retrying</span></span>
  </div>

  <section class="panel">
    <div class="auto">
      <div class="txt">
        <div class="title">Motion automation</div>
        <div class="desc" id="autoDesc">Plays a random animation when someone uses the stairs.</div>
      </div>
      <button class="switch" id="autoSwitch" role="switch" aria-checked="true" aria-label="Motion automation"></button>
    </div>
  </section>

  <section class="panel">
    <div class="head"><h2>Play an animation</h2><span class="hint">Runs 10 s (Matrix 60 s), then the stairs go back</span></div>
    <div class="scenes" id="scenes">
      <button class="scene" data-anim="2"><div class="sw sw-rainbow"></div><div class="nm">Rainbow</div></button>
      <button class="scene" data-anim="4"><div class="sw sw-stars"></div><div class="nm">Star sparkle</div></button>
      <button class="scene" data-anim="5"><div class="sw sw-bday"></div><div class="nm">Birthday</div></button>
      <button class="scene" data-anim="6"><div class="sw sw-night"></div><div class="nm">Night red</div></button>
      <button class="scene" data-anim="3"><div class="sw sw-white"></div><div class="nm">White ramp</div></button>
      <button class="scene" data-anim="7"><div class="sw sw-matrix" aria-hidden="true"></div><div class="nm">Matrix</div></button>
    </div>
  </section>

  <section class="panel">
    <div class="head"><h2>Fixed colour</h2><span class="hint">Red, green, blue and white LEDs</span></div>
    <div class="swatch-bar" id="mixBar"></div>
    <div class="ch"><label for="slR">R</label><input type="range" id="slR" min="0" max="100" value="0"><output id="oR">0%</output></div>
    <div class="ch"><label for="slG">G</label><input type="range" id="slG" min="0" max="100" value="0"><output id="oG">0%</output></div>
    <div class="ch"><label for="slB">B</label><input type="range" id="slB" min="0" max="100" value="0"><output id="oB">0%</output></div>
    <div class="ch"><label for="slW">W</label><input type="range" id="slW" min="0" max="100" value="0"><output id="oW">0%</output></div>
    <div class="presets"><span>All</span>
      <button class="pill" data-all="0">Off</button><button class="pill" data-all="25">25%</button><button class="pill" data-all="50">50%</button><button class="pill" data-all="75">75%</button><button class="pill" data-all="100">100%</button>
    </div>
    <div class="note" id="autoNote">Shown on the stairs once motion automation is off.</div>
  </section>

  <section class="panel">
    <div class="head"><h2>Recent motion</h2><span class="hint">Last 5</span></div>
    <ul class="log" id="log"></ul>
  </section>

  <section class="panel">
    <div class="head"><h2>Device</h2><span class="hint" id="upHint"></span></div>
    <div class="stats">
      <div class="stat"><div class="k">Memory</div><div class="v" id="memPct">--</div><div class="mini" id="memSub"></div><div class="bar"><i id="memBar"></i></div></div>
      <div class="stat"><div class="k">Flash</div><div class="v" id="flashPct">--</div><div class="mini" id="flashSub"></div><div class="bar"><i id="flashBar"></i></div></div>
      <div class="stat"><div class="k">WiFi signal</div><div class="v" id="wifiQ">--</div><div class="mini" id="wifiSub"></div><div class="bars" id="wifiBars"><i style="height:5px"></i><i style="height:8px"></i><i style="height:12px"></i><i style="height:16px"></i></div></div>
      <div class="stat"><div class="k">Last restart</div><div class="v" id="lastRestart">--</div><div class="mini" id="resetReason"></div></div>
    </div>
    <table class="kv" style="margin-top:12px"><tbody id="sysBody"></tbody></table>
  </section>

  <details class="panel sect">
    <summary><h2>Settings</h2><span class="chev" aria-hidden="true">&#9662;</span></summary>
    <div class="body">
      <div class="field"><label for="setHost">Device name on the network</label><input class="inp" id="setHost" maxlength="31" autocomplete="off"><span class="mini">Takes effect after a restart</span></div>
      <div class="auto">
        <div class="txt"><div class="title" style="font-size:14px">Night mode</div><div class="desc">Soft red light only, no animations</div></div>
        <button class="switch" id="nightSwitch" role="switch" aria-checked="false" aria-label="Night mode"></button>
      </div>
      <div class="row">
        <div class="field" style="flex:1"><label for="nStart">From</label><select class="inp" id="nStart"></select></div>
        <div class="field" style="flex:1"><label for="nEnd">Until</label><select class="inp" id="nEnd"></select></div>
      </div>
      <div class="row"><button class="btn" id="saveSet">Save settings</button><span class="status" id="setStatus"></span></div>
    </div>
  </details>

  <details class="panel sect">
    <summary><h2>Birthdays</h2><span class="chev" aria-hidden="true">&#9662;</span></summary>
    <div class="body">
      <div class="mini">On these days motion plays the birthday animation. Up to 20.</div>
      <div id="bdays" style="display:flex;flex-direction:column;gap:8px"></div>
      <div class="row"><button class="btn ghost" id="addBday">Add birthday</button><button class="btn" id="saveBday">Save birthdays</button><span class="status" id="bdayStatus"></span></div>
    </div>
  </details>

  <section class="panel">
    <div class="row" id="restartRow"><button class="btn danger" id="restartBtn">Restart device</button><span class="mini">Lights pause for about 10 s</span></div>
    <div class="confirm" id="restartConfirm" hidden><span style="flex:1;font-size:13.5px">Restart now?</span><button class="btn danger" id="restartYes">Restart</button><button class="btn ghost" id="restartNo">Cancel</button></div>
  </section>

  <footer>Firmware v)html" FW_VERSION R"html(</footer>
</div>

<script>
(function(){
function $(id){return document.getElementById(id);}
var st={auto:true,manual:[0,0,0,0],night:false,set:null,drag:[0,0,0,0],held:-1e9,clickAt:-1e9};
function hold(){st.held=performance.now();}   /* ignore polled auto/colour briefly after a local change */
var NAMES={2:'Rainbow',3:'White ramp',4:'Star sparkle',5:'Birthday',6:'Night red',7:'Matrix'};
var BYNAME={'Rainbow':2,'White ramp':3,'Star sparkle':4,'Birthday':5,'Night red':6,'Matrix':7};
var DOT={2:'linear-gradient(90deg,#ff3b5c,#3bd2ff)',3:'#fff1d6',4:'#6f8cff',5:'#ff4f7b',6:'#ff2a2a',7:'#3dff6a'};
function post(u,b){return fetch(u,{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:b||''});}
function getJ(u){return fetch(u,{cache:'no-store'}).then(function(r){return r.json();});}
function el(tag,cls,txt){var e=document.createElement(tag);if(cls)e.className=cls;if(txt!=null)e.textContent=txt;return e;}

/* ---------- animations: the tile of what is playing gets a progress ring ---------- */
var playing=0;
function markPlaying(id,pct){
  if(playing&&playing!==id){var o=document.querySelector('.scene[data-anim="'+playing+'"]');if(o){o.classList.remove('playing');var r0=o.querySelector('.ring');if(r0)r0.remove();}}
  playing=id;if(!id)return;
  var b=document.querySelector('.scene[data-anim="'+id+'"]');if(!b)return;
  b.classList.add('playing');var r=b.querySelector('.ring')||b.appendChild(el('span','ring'));r.style.setProperty('--p',pct);}
document.querySelectorAll('.scene').forEach(function(b){b.addEventListener('click',function(){
  var id=+b.dataset.anim;post('/api/play','anim='+id);st.clickAt=performance.now();markPlaying(id,0);});});

/* ---------- automation ---------- */
function showAuto(on){st.auto=on;$('autoSwitch').setAttribute('aria-checked',on);
  $('autoDesc').textContent=on?'Plays a random animation when someone uses the stairs.':'Off. The stairs show the fixed colour below.';
  $('autoNote').hidden=!on;}
function setAuto(on){hold();showAuto(on);return post('/api/auto','on='+(on?1:0));}
$('autoSwitch').addEventListener('click',function(){setAuto(!st.auto);});

/* ---------- fixed colour ---------- */
var sl=['slR','slG','slB','slW'].map($),outs=['oR','oG','oB','oW'].map($),CH=['r','g','b','w'],tmr=[];
function mixPreview(){var v=st.manual.map(function(p){return p/100;}),
  c=[Math.min(1,v[0]+v[3]),Math.min(1,v[1]+v[3]*.94),Math.min(1,v[2]+v[3]*.84)].map(function(x){return Math.round(255*Math.pow(x,.8));});
  $('mixBar').style.setProperty('--mix','rgb('+c.join(',')+')');}
function showManual(vals){st.manual=vals.slice();sl.forEach(function(s,i){s.value=vals[i];outs[i].textContent=vals[i]+'%';});mixPreview();}
function setManual(vals){hold();showManual(vals);
  if(vals[0]===vals[1]&&vals[1]===vals[2]&&vals[2]===vals[3])return post('/api/color','all='+vals[0]);
  return CH.reduce(function(pr,ch,i){return pr.then(function(){return post('/api/color','c='+ch+'&v='+vals[i]);});},Promise.resolve());}
sl.forEach(function(s,i){s.addEventListener('input',function(){
  var v=st.manual.slice();v[i]=+s.value;hold();showManual(v);st.drag[i]=1;clearTimeout(tmr[i]);
  tmr[i]=setTimeout(function(){post('/api/color','c='+CH[i]+'&v='+s.value).then(function(){st.drag[i]=0;});},120);});});
document.querySelectorAll('.pill[data-all]').forEach(function(b){b.addEventListener('click',function(){var v=+b.dataset.all;setManual([v,v,v,v]);});});

/* ---------- polling ---------- */
function fmtDate(d){if(!d||d==='--')return'--';var x=new Date(d.replace(' ','T'));return isNaN(x)?d:x.toLocaleDateString('en-GB',{weekday:'short',day:'numeric',month:'short'});}
function fmtUp(ms){var d=Math.floor(ms/864e5),h=Math.floor(ms%864e5/36e5),m=Math.floor(ms%36e5/6e4);return d?d+' d '+h+' h':h+' h '+m+' min';}
function nightChip(){var c=$('chipNight'),s=st.set;if(!s||!s.night_enabled){c.hidden=true;return;}c.hidden=false;
  c.querySelector('span').textContent=st.night?'Night mode active':'Night mode '+s.night_start+':00 to '+s.night_end+':00';}
function loadFast(){
  Promise.all([getJ('/api/state'),getJ('/api/time')]).then(function(a){var s=a[0],t=a[1];
    st.night=!!s.night;var fresh=performance.now()-st.held>2500;if(fresh&&(s.auto==1)!==st.auto)showAuto(s.auto==1);
    var m=[s.r,s.g,s.b,s.w];if(fresh&&!st.drag.some(Boolean)&&m.join()!==st.manual.join())showManual(m);
    if(s.anim||performance.now()-st.clickAt>2500)markPlaying(s.anim||0,s.anim?Math.min(100,s.anim_ms/s.anim_len*100):0);
    nightChip();
    $('clockTime').textContent=t.time&&t.time!=='--'?t.time.slice(0,5):'--:--';$('clockDate').textContent=fmtDate(t.date);
    $('upHint').textContent='Up '+fmtUp(t.uptime_ms||0);
    var lr=t.last_reboot;if(lr&&lr!=='--'){var x=new Date(lr.replace(' ','T'));$('lastRestart').textContent=isNaN(x)?lr:x.toLocaleDateString('en-GB',{weekday:'short'})+' '+lr.slice(11,16);}
    $('chipConn').hidden=true;
  }).catch(function(){$('chipConn').hidden=false;});
}
function row(tb,k,v){var r=el('tr');r.appendChild(el('td',null,k));r.appendChild(el('td',null,v));tb.appendChild(r);}
function kb(n){return n>=1048576?(n/1048576).toFixed(1)+' MB':n>=1024?(n/1024).toFixed(1)+' KB':n+' B';}
var mem=null;
function loadSlow(){
  getJ('/api/log').then(function(lg){var ul=$('log');ul.textContent='';
    if(!lg.length){ul.appendChild(el('li','empty','No motion since the last restart.'));return;}
    lg.forEach(function(e){var up=e.dir!=='DOWN',id=BYNAME[e.anim]||6,li=el('li'),d=el('span','dir');
      d.innerHTML=up?'<svg viewBox="0 0 24 24" fill="none" stroke-width="2.2" stroke-linecap="round" stroke-linejoin="round"><path d="M12 19V5M6 11l6-6 6 6"/></svg>':'<svg viewBox="0 0 24 24" fill="none" stroke-width="2.2" stroke-linecap="round" stroke-linejoin="round"><path d="M12 5v14M6 13l6 6 6-6"/></svg>';
      var mid=el('span');mid.appendChild(el('div','what',up?'Up the stairs':'Down the stairs'));
      var sub=el('div','sub'),dot=el('span','dot');dot.style.background=DOT[id];sub.appendChild(dot);sub.appendChild(document.createTextNode(NAMES[id]));mid.appendChild(sub);
      li.appendChild(d);li.appendChild(mid);li.appendChild(el('time',null,e.time));ul.appendChild(li);});}).catch(function(){});
  getJ('/api/memory').then(function(m){mem=m;$('memPct').textContent=m.heap_pct+'%';$('memBar').style.width=m.heap_pct+'%';
    $('flashPct').textContent=m.flash_pct+'%';$('flashBar').style.width=m.flash_pct+'%';
    $('memSub').textContent=kb(m.heap_used)+' of '+kb(m.heap_total);$('flashSub').textContent=kb(m.flash_used)+' of '+kb(m.flash_size);}).catch(function(){})
  .then(function(){return getJ('/api/sysinfo');}).then(function(s){
    var q=s.rssi>=-50?['Excellent',4]:s.rssi>=-60?['Good',3]:s.rssi>=-70?['Fair',2]:['Poor',1];
    $('wifiQ').textContent=q[0];$('wifiSub').textContent=s.rssi+' dBm';[].forEach.call($('wifiBars').children,function(b,i){b.classList.toggle('on',i<q[1]);});
    $('ssid').textContent=s.ssid;$('resetReason').textContent=s.reset_reason;
    var tb=$('sysBody');tb.textContent='';
    row(tb,'IP address',s.ip);row(tb,'Gateway',s.gateway);row(tb,'DNS',s.dns);row(tb,'Network',s.ssid+', channel '+s.channel);
    row(tb,'Access point',s.bssid);row(tb,'Reconnects',String(s.reconnects));row(tb,'CPU',s.cpu_mhz+' MHz');
    row(tb,'Last restart reason',s.reset_reason);if(String(s.reset_reason).indexOf('xception')>=0)row(tb,'Exception info',s.reset_info);
    if(mem){row(tb,'Memory free',kb(mem.heap_free));row(tb,'Largest free block',kb(mem.heap_max_block));row(tb,'Memory fragmentation',mem.heap_frag+'%');row(tb,'Space for updates',kb(mem.sketch_free));}}).catch(function(){});
}

/* ---------- settings ---------- */
['nStart','nEnd'].forEach(function(id){for(var h=0;h<24;h++){var o=el('option',null,h+':00');o.value=h;$(id).appendChild(o);}});
function loadSettings(){getJ('/api/settings').then(function(s){st.set=s;$('setHost').value=s.hostname;$('hostName').textContent=s.hostname;
  $('nightSwitch').setAttribute('aria-checked',!!s.night_enabled);$('nStart').value=s.night_start;$('nEnd').value=s.night_end;nightChip();}).catch(function(){});}
$('nightSwitch').addEventListener('click',function(e){var b=e.currentTarget;b.setAttribute('aria-checked',b.getAttribute('aria-checked')!=='true');});
function status(id,ok,txt){var s=$(id);s.textContent=txt;s.style.color=ok?'var(--ok)':'var(--bad)';if(ok)setTimeout(function(){if(s.textContent===txt)s.textContent='';},3000);}
function saveResult(id,r,after){if(r.status===204){status(id,true,'Saved');after();}else r.text().then(function(t){status(id,false,'Not saved: '+t);});}
$('saveSet').addEventListener('click',function(){status('setStatus',true,'Saving...');
  post('/api/settings','hostname='+encodeURIComponent($('setHost').value.trim())+'&night_enabled='+($('nightSwitch').getAttribute('aria-checked')==='true'?1:0)+'&night_start='+$('nStart').value+'&night_end='+$('nEnd').value)
  .then(function(r){saveResult('setStatus',r,loadSettings);}).catch(function(){status('setStatus',false,'Not saved: no connection');});});

/* ---------- birthdays ---------- */
function bdayRow(m,d,n){var r=el('div','bday');
  var im=el('input','inp');im.type='number';im.min=1;im.max=12;im.placeholder='MM';im.setAttribute('aria-label','Month');im.value=m;
  var idd=el('input','inp');idd.type='number';idd.min=1;idd.max=31;idd.placeholder='DD';idd.setAttribute('aria-label','Day');idd.value=d;
  var inm=el('input','inp');inm.placeholder='Name';inm.maxLength=19;inm.setAttribute('aria-label','Name');inm.value=n;
  var x=el('button','x','×');x.setAttribute('aria-label','Remove');x.addEventListener('click',function(){r.remove();});
  [im,idd,inm,x].forEach(function(c){r.appendChild(c);});return r;}
function loadBirthdays(){getJ('/api/birthdays').then(function(a){var b=$('bdays');b.textContent='';a.forEach(function(x){b.appendChild(bdayRow(x.m,x.d,x.name));});}).catch(function(){});}
$('addBday').addEventListener('click',function(){if($('bdays').children.length<20)$('bdays').appendChild(bdayRow('','',''));});
$('saveBday').addEventListener('click',function(){var rows=$('bdays').children,b='count='+rows.length;
  for(var i=0;i<rows.length;i++){var f=rows[i].querySelectorAll('input');b+='&m'+i+'='+encodeURIComponent(f[0].value)+'&d'+i+'='+encodeURIComponent(f[1].value)+'&n'+i+'='+encodeURIComponent(f[2].value.trim());}
  status('bdayStatus',true,'Saving...');post('/api/birthdays',b).then(function(r){saveResult('bdayStatus',r,loadBirthdays);}).catch(function(){status('bdayStatus',false,'Not saved: no connection');});});

/* ---------- restart ---------- */
$('restartBtn').addEventListener('click',function(){$('restartRow').hidden=true;$('restartConfirm').hidden=false;});
$('restartNo').addEventListener('click',function(){$('restartRow').hidden=false;$('restartConfirm').hidden=true;});
$('restartYes').addEventListener('click',function(){$('restartConfirm').hidden=true;$('restartRow').hidden=false;var b=$('restartBtn');b.textContent='Restarting...';b.disabled=true;
  post('/api/reboot').then(function(){setTimeout(function(){location.reload();},8000);});});

/* ---------- Matrix tile: falling glyph columns ---------- */
(function(){var sw=document.querySelector('.sw-matrix'),G='ﾊﾐﾋｰｳｼﾅﾓﾆｻﾜﾂｵﾘｱﾎﾃﾏｹﾒｴｶｷﾑﾕﾗｾﾈｽﾀﾇﾍ012345789Z',cols=[];
  function g(){return G.charAt(Math.random()*G.length|0);}
  for(var i=0;i<13;i++){var c=el('i','mx'),t='';for(var k=0;k<12;k++)t+=g();c.textContent=t;
    c.style.left=(i*7.7+Math.random()*2)+'%';c.style.animationDuration=(1.4+Math.random()*2.4)+'s';c.style.animationDelay=(-Math.random()*4)+'s';
    sw.appendChild(c);cols.push(c);}
  if(!matchMedia('(prefers-reduced-motion: reduce)').matches)setInterval(function(){   /* characters change while falling */
    var c=cols[Math.random()*cols.length|0],t=c.textContent,k=Math.random()*t.length|0;c.textContent=t.slice(0,k)+g()+t.slice(k+1);},60);
})();
mixPreview();
loadSettings();loadBirthdays();loadFast();loadSlow();setInterval(loadFast,2000);setInterval(loadSlow,15000);
})();
</script>
</body></html>
)html";

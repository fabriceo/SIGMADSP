var app = new Vue({
    el: '#app',
    data: {
        savingMsg: "Save changes ?",
        toggleSaveModal: 0, //carreful don't already display none by CSS
        volume: {
            max: 0,
            min: -100,
        },

        dspstatus: { //default value, but it's bidirectional bind
            volume: -40,
            source: 1,
            config: 1,
            mute: 0,
            configchanged: 0,
            sourcenames: [
                "in1",
                "in2",
                "in3",
            ],
            confignames: [
                "cfg1",
                "cfg2",
            ],
        },
    },
    methods: {
        bsave: function () {
            this.savingMsg = "please wait, updating server ...";
        },
        bdiscard: function () {
            this.savingMsg = "please wait, reloading from server ...";
        },
    },
    mounted: function () {
        // used to schedule the async interraction with server,
        // and possible check time out, and saveall sequence
        setInterval( periodicPoolling, 100);
    }
});

//some class, TODO: should be manage with bind variable
const bunsel = "w3-button w3-light-blue w3-xlarge w3-ripple w3-wide w3-border";
const bsel   = "w3-button w3-blue w3-xlarge w3-ripple w3-wide w3-border";
const trunsel= "w3-light-grey w3-hover-light-blue";

var modeLocal = 0;        // 0 : expecting server. 1 : no server required, default objects
const pullingStatusDivider  = 2;   // time base is 100ms, 2 means we send /getdspstatus every 200ms only
const timeoutConnectionLost = 20;  // 2sec
const tempoTryReconnect     = 100; // 10 sec

const Fmin = 10,  Fdef = 1000, Fmax = 40000;
const Qmin = 0,   Qdef = 0,    Qmax = 99;
const Gmin = -20, Gdef = 0,    Gmax = 20;
const Dmin = 0,   Ddef = 0,    Dmax = 100000;

const celerity = 343;     // sound speed in m/s
var samplesmax = 1500;    // max number of samples for the dsp channel delay line
var samplingfreq = 192000;  // max sampling freq supported

var inputs;             // number of inputs chanel based on the information from dspconfig
var outputs;            // number of inputs chanel based on the information from dspconfig
var connected=0;        // 1 if the connection exist with the server. detected by onload() and getsttus
var dspstatus;
var dspconfig;          // tables for the channels configuration names,gain, mute,invert,delays
var dspchannel;         // filter configuration of the current channel under edition
var inputsbasic = 1;    // set 1 if inputs are only gain+eq (no hp, no lp, no delay)

const func1 = "Channel";    // at least the Channel button is visible in the function pannel
var modedelayus = 1;    // by default, delays are typed in microsec

var channel = 0;        // current channel number selected on screen
var selfunc = 0;        // current value of the function selector index (G,EQ,HP,LP,D) on screen
var funclist = [func1];  // list of supported function ("MIXER","EQ","HP","LP","FIR")

var filters;            // point on one of the dspchannel.EQ/HP/LP object according to selfunc
var filtersmax;         // max number of filters according to selfunc
var limitbiquads;       // maximum number of filters to be defined within the biquad cell capacity
var mixer;              // point on the mixer array within dspchannel, when corresponding selfunc is selected

var channelmodified = 0;    // 1 when any of the filters for the ongoing channel is modified
var configmodified  = 0;    // 1 when any of the tables of the dspconfig is modified
var statusmodified  = 0;    // 1 when source, config, volume... is modified
var modalSaveYesNo;

var selEQ, selHP, selLP, selFIR, selMIXER;  // > 0  if the function exists in the menu


class Filter {
    constructor(T,F,Q,G,I,B){
        this.F=F; this.T=T; this.Q=Q; this.G=G; this.I=I; this.B=B;}  }


var dspstatus_default = {
    source: 1, config: 1, volume: -400, mute: 0,
    configchanged: 0,
    sourcenames: ["in1", "in2","in3"],
    confignames: ["cfg1", "cfg2"],
}

var dspconfig_default = {
    num:    1,
    names:  ["left", "right", "CH1", "CH2", "CH3", "CH4"],  // channels name
    links:  [ 0, 1, 0, 0, 0, 0],
    mixers: [  1,0, 0,1, 1,0, 1,0, 0,1, 0,1 ],
    gains:  [ -1.0, 0.0, -1.1, 1.2, -1.3, 1.4],
    mutes:  [0, 0, 0, 0, 0, 0],
    inverts:[0, 0, 0, 0, 0, 0],
    delays: [0, 0, 900, 150, 902, 152],
    firfiles: ["","","","firmid","",""],
    HPmax: 4, LPmax: 4, EQmax: 6 }

var dspchannel_default = {
    num: 1,
    HP:  [ new Filter("HPBU2", 30,  0,   0,   0, 0),
        new Filter("HP2  ", 20,  0.7, 0,   0, 0) ],
    LP:  [ new Filter("LPLR4", 400, 0,   0,   0, 0) ],
    EQ:  [ new Filter("PEQ  ", 125, 1.5, 3.0, 0, 0),
        new Filter("NOTCH", 63,  20,  4,   0, 0) ] };


const filtersinfo = {
    names: ["NONE ","HP1  ","HP2  ","HPSUB", "HPBE1","HPBE2","HPBE3","HPBE4","HPBE6","HPBE8", "HPBU1","HPBU2","HPBU3","HPBU4","HPBU6","HPBU8", "HPLR2","HPLR4","HPLR6","HPLR8",
        "LP1  ","LP2  ", "LPBE1","LPBE2","LPBE3","LPBE4","LPBE6","LPBE8", "LPBU1","LPBU2","LPBU3","LPBU4","LPBU6","LPBU8", "LPLR2","LPLR4","LPLR6","LPLR8",
        "PEQ  ","NOTCH","ALLP1","ALLP2","LSH1 ","LSH2", "HSH1 ","HSH2 ","BP0DB","BPQ  " ],
    order:   [1,1,2,2,1,2,3,4,6,8,1,2,3,4,6,8,2,4,6,8, 1,2,1,2,3,4,6,8,1,2,3,4,6,8,2,4,6,8, 2,2,1,2,1,2,1,2,2,2 ],
    cells:   [1,1,1,2,1,1,2,2,3,4,1,1,2,2,3,4,1,2,3,4, 1,1,1,1,2,2,3,4,1,1,2,2,3,4,1,2,3,4, 1,1,1,1,1,1,1,1,1,1 ],
    Qneeds:  [0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 1,1,0,1,0,1,0,1,1,1 ],
    Gneeds:  [0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 1,0,0,0,1,1,1,1,1,1 ]
}


// calculate the number of input channels and output channels 
// based on the mixers table length vs gains table  length
function calcConfigInputsOutputs(){
    outputs = dspconfig.gains.length;
    inputs  = dspconfig.mixers.length / outputs;
    outputs -= inputs;
}

// HELPERS functions to limit external libraries

function id(name){
    return document.getElementById(name); }

function defined(x){
    if (typeof x === 'undefined') return false; else return true; }

function setClass(x,cl){
    x.setAttribute( "class", cl); }

function setVisible(x){
    x.style.visibility = "visible"; }

function setHidden(x){
    x.style.visibility = "hidden"; }

function setDisplayNone(x){
    x.style.display = "none"; }

function setDisplayBlock(x){
    x.style.display = "block"; }


// verify if dspconfig or dspchannel have changed since last saved
function checkModified(){
    if (configmodified || channelmodified) {
        id("savingmsg").innerHTML = "Save changes ?";
        modalSaveYesNo = 1; // indicate modal is open 
        this.toggleSaveModal = 1;  // open modal dialog box
        return 0;   // by returning 0, the original requestor should cancel the onchange action requested
    } else return 1; }

// update visibility of the panel function for the C/M/EQ/HP/LP functions
function function_panel_display() {
    var divfunc = id("divfunc");
    if (channel)  {
        setDisplayBlock(divfunc);
        var divchannel = id("divchannel");
        var divfilter  = id("divfilter");
        var divmixer   = id("divmixer");
        var divfir     = id("divfir");
        if (selfunc == 1) {
            setDisplayBlock(divchannel);
            channelfields_update();
        } else setDisplayNone(divchannel);
        if (selfunc == selMIXER) setDisplayBlock(divmixer);
        else setDisplayNone(divmixer);
        if ((selfunc==selEQ)||(selfunc==selHP)||(selfunc==selLP)) setDisplayBlock(divfilter);
        else setDisplayNone(divfilter);
        if ((selfunc==selFIR)) setDisplayBlock(divfir);
        else setDisplayNone(divfir);
    } else
        setDisplayNone(divfunc);
}

// generate the HTML <inputs> element for the mixer panel
function updateInputMixers() {
    var i;
    var tr1   = id("trmixer1");  // model
    var tbody = id("trmixer_");  // empty area to fill with model
    var ht = '';
    for (i=2; i<= inputs; i++)
        ht += '<tr id="trmixer'+i+'" onmousedown="mixclick('+i+')">' + tr1.innerHTML.replace(/1/g,i) + '</tr>\n';
    tbody.innerHTML = ht;
}

// display the N lines for changing the mixer parameters
function displayMixerLines(){
    for (i=1; i <= inputs; i++) {
        var trmix = id("trmixer"+i);
        if(defined(trmix)) {
            setClass(trmix, trunsel);
            var ch = id("mixname"+i);
            ch.innerHTML = dspconfig.names[i-1];
            var mix = id("mix"+i);
            mix.value = parseFloat(mixer[i-1]); }
    }
}


// display a blue (selecetd) or greyline for each filter line.
// and adjust the background of the F/Q/G input buttons
function displayFiltersLines(){
    for (i=1; i <= filtersmax; i++) {
        var trfilt = id("trfilt"+i);
        if (i <= limitbiquads) {
            setVisible(trfilt);
            setClass(trfilt, trunsel);
        } else {
            setHidden(trfilt); setHidden(id("fin"+i)); setHidden(id("qin"+i));
            setHidden(id("gin"+i)); setHidden(id("bin"+i)); setHidden(id("iin"+i));  }
    }
}

function calcLimitBiquads(){
    var i,biquads = 0;
    for (i = 1; i <= filtersmax; i++)
        if (biquads < filtersmax) {
            limitbiquads = i;
            biquads += findCells(id("ftype"+i).value);
        } else break;
}

// display all the filter line and disable un-needed or unpossible element
function displayFilters(){
    calcLimitBiquads();
    for (i=1; i <= limitbiquads; i++) {
        var T = id("ftype"+i); // filter type
        var F = id("fin"+i);   // deselect frequency if NONE
        var B = id("bin"+i);   // bypass
        var I = id("iin"+i);   // invert
        var name = T.value;    // filter type name
        var idx = filtersinfo.names.indexOf(name);  // retreive invex value
        if (idx<=0) {
            idx = 0;
            setHidden(F); setHidden(B); setHidden(I); }
        else {
            setVisible(F); setVisible(B); setVisible(I); }

        var Q = id("qin"+i);       // Q
        if (filtersinfo.Qneeds[idx])  setVisible(Q); else setHidden(Q);

        var G = id("gin"+i);       // G
        if (filtersinfo.Gneeds[idx])  setVisible(G); else setHidden(G);
    }
    displayFiltersLines();
}

function updateConfigInfo(blabla) {
    var text = '('+document.body.clientWidth+'px) '+(modeLocal) ? "local " : "";

    text += (connected ? "connected " : "no server ");
    if (connected) text+= "config:"+dspstatus.config+" "+dspstatus.confignames[dspstatus.config-1] + ", inputs="+inputs+", outputs="+outputs;
    id("topinfo").innerHTML = text + blabla;  }

function update_channelinfo(blabla){
    id("channelinfo").innerHTML = blabla; }

function updateSourceConfigButtonsVolume(){
    if (dspstatus) {
        // update source button name and color
        var N = dspstatus.sourcenames.length;   // number of possible sources
        var percent = Math.floor(95/N);         // distribute size across buttons
        var ht = '<tr><td>';
        var but = '<button id="bsource_" onclick="bsource(_)" style="width:'+percent+'%" class="';
        for (let i=1; i<= N; i++) {
            ht += but.replace(/_/g,i);
            ht += ( i == dspstatus.source ? bsel : bunsel );
            ht+= '">'+dspstatus.sourcenames[i-1]+'</button>'; }
        ht += '</td></tr><tr><td><br></td></tr><tr><td>';
        // update config button name and color    N = dspstatus.confignames.length;
        N = dspstatus.confignames.length;
        percent = Math.floor(95/N);
        but = '<button id="bpres_" onclick="bpreset(_)" style="width:'+percent+'%" class="';
        for (let i=1; i <= N; i++) {
            ht += but.replace(/_/g,i);
            ht += ( i == dspstatus.config ? bsel : bunsel );
            ht+= '">'+dspstatus.confignames[i-1]+'</button>'; }
        id("tbodysource").innerHTML = ht + '</td></tr>';

        setDisplayBlock(id("pageready"));
    } else
        updateConfigInfo("...");
}

function updateChannelButtons(){
    var Linputs, Loutputs, Cinputs, Coutputs, Lmax, Cmax;

    if (inputs<=2)       { Cinputs=1; Linputs=2; }
    else if (inputs<=8)  { Cinputs=2; Linputs=Math.floor((inputs+1)/2);  }
    else if (inputs<=12) { Cinputs=3; Linputs=4; }
    else { Cinputs=4; Linputs = Math.floor((inputs+3)/4);}

    Coutputs = Math.floor((outputs+1)/2);
    if (Coutputs>4) Coutputs=4;
    if (outputs<=8) Loutputs=2;
    else Loutputs=Math.floor((outputs+3)/4);
    Cmax = Cinputs+Coutputs;
    if (Loutputs>Linputs) Lmax = Loutputs; else Lmax = Linputs;

    var width = Math.floor(98/Cmax);
    var input=0,output=0;
    var ht = '';
    var but = '<button id="bchan_" onclick="bchannel(_)" style="width:'+width+'%" class="';
    for (let l=1; l<=Lmax; l++){ ht += '<tr>';
        for (c=1; c<=Cmax; c++) { ht += '<td>'; var x;
            if (c <= Cinputs) {
                input++; if (input<=inputs) x=input; else x=0;
            } else {
                output++; if (output<=outputs) x=inputs+output; else x=0; }
            if (x) {
                ht += but.replace(/_/g, x);
                ht += ( x === channel ? bsel : bunsel );
                ht+= '">'+dspconfig.names[x-1]+'</button>'; }
            ht+='</td>'; }
        ht += '</tr>'; }
    id("tbodychannel").innerHTML = ht;
}

// update most of the DOM elements on the screen, 
// depending on source/config/channel/function selected

function DOM_update() {
    DOMrefresh = 0;
    updateSourceConfigButtonsVolume();
    updateChannelButtons();

    // prepare the variable to handle the list of function buttons
    if (dspchannel && channel) {

        update_channelinfo("editing channel "+channel+((channelmodified+configmodified)? " (modified)":""));

        funclist = [func1];
        if (defined(dspconfig.mixers)) {
            funclist.push("Mixer");
            selMIXER = funclist.length;
            mixer = dspconfig.mixers; }
        if (defined(dspchannel.EQ)) {
            funclist.push("EQ");
            selEQ = funclist.length; }
        if (defined(dspchannel.HP)) {
            funclist.push("HiPass");
            selHP = funclist.length; }
        if (defined(dspchannel.LP)) {
            funclist.push("LoPass");
            selLP = funclist.length; }
        if (defined(dspconfig.firfiles)) {
            funclist.push("FIR");
            selFIR = funclist.length; }
    } else
        update_channelinfo("");

    // create functions button (Channel, EQ, Mixer, EQ, HP, LP..)
    var tbody = id("tbodyfunc");  // empty area to fill with model
    var ht = '';
    var but = '<button id="bfunc_" onclick="bfunc(_)" class="';
    for (var i=1; i<= funclist.length; i++) {
        ht += but.replace(/_/g,i);
        ht += ( i === selfunc ? bsel : bunsel );
        ht+= '">'+funclist[i-1]+'</button>'; }
    tbody.innerHTML = ht;

    // display a panel below functions button
    function_panel_display();

}


// return number of biquad cells required by a filter type name
function findCells(name){
    var idx = filtersinfo.names.indexOf(name);
    if (idx<0) idx = 0;
    return filtersinfo.cells[idx];
}

// update the left filter type <select> buttons
// called only when changing/selecting the "function" button G/EQ/HP/LP
function updateSelectFilters() {
    var x, y, i;
    var tr1   = id("trfilt1");  // model
    var tbody = id("trfilt_");  // empty area to fill with model
    var ht = '';
    for (i=2; i<= filtersmax; i++)
        ht += '<tr id="trfilt'+i+'" onmousedown="filtclick('+i+')">' + tr1.innerHTML.replace(/1/g,i) + '</tr>\n';
    tbody.innerHTML = ht;
    if (selfunc == selEQ) {
        x =  id("eqfilter");
        y = x; }
    if (selfunc == selHP) {
        x =  id("hpfilter");
        y =  id("hpfilterbase");  }
    if (selfunc == selLP) {
        x =  id("lpfilter");
        y =  id("lpfilterbase");  }
    id("ftype1").innerHTML = x.innerHTML;   // define button option for first <select>
    for (i=2; i<= filtersmax; i++)       // copy <select> <OPTIONS> in other selectors
        id("ftype"+i).innerHTML = y.innerHTML;
}

function checkF(F){
    if (isNaN(F)) F = Fdef; if (F<Fmin) F=Fmin; if (F>Fmax) F=Fmax; return F; }

function checkQ(Q) {
    if (isNaN(Q)) Q = Qdef; if (Q<Qmin) Q = Qmin; if (Q>Qmax) Q = Qmax; return Q; }

function checkG(G){
    if (isNaN(G)) G = Gdef; if (G<Gmin) G = Gmin; if (G>Gmax)  G = Gmax; return G; }

function shrinkFilter(f){
    var idx = filtersinfo.names.indexOf(f.T);  // find the filter attributes for the given filter
    if (idx<0) { idx = 0; f.T = "NONE "; }
    if (idx) f.F=checkF(f.F); else delete f.F;
    if (filtersinfo.Qneeds[idx]) f.Q=checkQ(f.Q); else delete f.Q;
    if (filtersinfo.Gneeds[idx]) f.G=checkG(f.G); else delete f.G;
    if (f.B) f.B = 1; else delete f.B;
    if (f.I) f.I = 1; else delete f.I;
}

// reduce the number of elements in the filter array, depending on filter types
function shrinkFilters(filters){
    var max = filters.length;
    for (i= max-1; i>0; i--) {
        let name = filters[i].T;
        if (name === "NONE ") filters.pop();     // remove the latest NONE filter
        else break; }
    for (let f in filters) shrinkFilter(f);
}

function shrinkChannelFilters(){
    if (defined(dspchannel.EQ)) shrinkFilters(dspchannel.EQ);
    if (defined(dspchannel.HP)) shrinkFilters(dspchannel.HP);
    if (defined(dspchannel.LP)) shrinkFilters(dspchannel.LP);
}

function expandFilter(f){
    if (!defined(f.T)) f.T = "NONE ";
    if (f.T === "NONE ") f.F = 0;
    else if (defined(f.F)) f.F=checkF(f.F); else f.F = Fdef;
    if (defined(f.Q)) f.Q=checkQ(f.Q); else f.Q = Qdef;
    if (defined(f.G)) f.G=checkG(f.G); else f.G = Gdef;
    if (!defined(f.I)) f.I = 0;
    if (!defined(f.B)) f.B = 0;
}

// make sure all the filter properties are populated in the filter array, to simplify editing
function expandFilters(filters, max){
    for (i=0; i < max; i++) {
        if (!defined(filters[i])) {
            var fnone = new Filter("NONE ",0,0,0,0,0);
            filters.push(fnone);
        } else
            expandFilter(filters[i])
    }
}

// select the filter array according to selfunc
function loadFilters(){
    if (selfunc == selEQ) {
        filters = dspchannel.EQ;
        filtersmax = dspconfig.EQmax }
    if (selfunc == selHP) {
        filters = dspchannel.HP;
        filtersmax = dspconfig.HPmax; }
    if (selfunc == selLP) {
        filters = dspchannel.LP;
        filtersmax = dspconfig.LPmax; }
    //filtercopy = JSON.parse(JSON.stringify(filters));
    expandFilters(filters, filtersmax);
    return filtersmax;
}


// load all the filter attributes in the <SELECT> and <BUTTONS>
// calculate the number of cells used by this configuration and hide unpossible <select>
function getFilters(){
    for (let i=1; i<= filters.length; i++) {
        id("ftype"+i).value = filters[i-1].T;
        id("fin"+i).value = parseInt(filters[i-1].F);
        id("qin"+i).value = parseInt(filters[i-1].Q);
        id("gin"+i).value = parseInt(filters[i-1].G);
        id("iin"+i).checked = (filters[i-1].I);
        id("bin"+i).checked = (filters[i-1].B); }
}

var periodicCount;              // incremented by 1 every 100ms
var HttpOngoing;                // 1 if an httprequest was just sent

var serverstatus, serverconfig, serverchannel;


function init(){                // called by onload
    if (modeLocal) {
        // local string object to simulate Server side, in local mode only
        serverstatus = JSON.stringify(dspstatus_default);
        serverconfig = JSON.stringify(dspconfig_default);
        serverchannel= JSON.stringify(dspchannel_default) }
    dspstatus = null;
    dspconfig = null;
    periodicCount = pullingStatusDivider;   // ensure that a getdspstatus is sent asap
    HttpOngoing = 0;
    connected = 0;
    channel   = 0;
    selfunc   = 0;
    statusmodified  = 0;
    configmodified  = 0;
    channelmodified = 0;
    modalSaveYesNo = 0;
    DOMrefresh     = 0;
}

function getHttp(url, str, callback) {
    if (modeLocal) {
        var data = JSON.parse(str);
        if (url != "/getdspstatus") console.log("GET "+url, data);
        callback(data);
        connected = 1; HttpOngoing = 0; periodicCount = 0;
    } else {
        var xhttp = new XMLHttpRequest();
        xhttp.onreadystatechange = function() {
            if(this.readyState == 4 && this.status == 200) {
                var data = JSON.parse(this.responseText);
                console.log("received ("+periodicCount+") ", data);
                callback(data);
                connected = 1; HttpOngoing = 0; periodicCount = 0; } };
        xhttp.open("GET", url, true);
        xhttp.send();
        if (url != "/getdspstatus") console.log("GET "+url);
        HttpOngoing = 1;
    }
}

function setHttp(url, source, callback) {
    if (modeLocal) {
        console.log("POST "+url, source);
        var data = JSON.stringify(source);
        callback();
        connected = 1; HttpOngoing = 0; periodicCount = 0;
        return data;
    } else {
        var xhttp = new XMLHttpRequest();
        xhttp.onreadystatechange = function() {
            if(this.readyState == 4 && this.status == 200) {
                callback();
                console.log("OK ("+periodicCount+")");
                connected = 1; HttpOngoing = 0; periodicCount = 0; }  };
        HttpOngoing = 1; periodicCount = 0;
        xhttp.open("POST", url, true);
        xhttp.setRequestHeader("Content-Type", "application/json");
        var data = JSON.stringify(source);
        xhttp.send(data);
        console.log("POST "+url,source); }
}

function periodicPoolling(){

    periodicCount++;

    if (HttpOngoing) { // already awaiting a server response
        // check time out
        if (connected) {
            if (periodicCount > timeoutConnectionLost) {// 2sec
                connected = 0;      // here we keep the HttpOngoing flag set to 1
                if (modalSaveYesNo)
                    this.toggleSaveModal = 0;  // close modal if it was open
                periodicCount = 0;
                //modeLocal = 1;init();
                console.log("CONNECTION LOST : timeout ", periodicCount); }
        } else
        if (periodicCount > tempoTryReconnect) // 10sec
            window.location.reload();
    } else {
        if (connected) {

            if ( statusmodified ) {     // requires updating the server
                statusmodified = 2;     // memorize the fact that we are trying to do it
                serverstatus = setHttp("/setdspstatus", dspstatus, // update server
                    function() {  if (statusmodified==2) statusmodified = 0; } ) ; // check if action was atomic or disturbed by cclient
            }
            if (channelmodified) {
                channelmodified = 2;
                serverchannel = setHttp("/setdspchannel?ch="+String(channel), dspchannel,
                    function() { if (channelmodified ==2) channelmodified = 0; });
            }
            if (configmodified) {
                configmodified = 2;
                serverconfig = setHttp("/setdspconfig",  dspconfig,
                    function() { if (configmodified==2) configmodified = 0;  });
            }

            if ( (dspconfig == null) || (dspstatus.config != dspconfig.num) ) {
                getHttp("/getdspconfig?data", serverconfig,
                    function(data) {
                        if (modeLocal) data.num = dspstatus.config;
                        if (configmodified == 0) { // avoid conflicting situation : prioritize client modification
                            dspconfig = data;
                            calcConfigInputsOutputs();
                            updateConfigInfo("");
                            DOMrefresh = 1;
                        } else
                            console.log("discard dspconfig received");
                    } ) ; }

            if ( channel && ( (dspchannel == null) || (channel != dspchannel.num) )) {
                getHttp("/getdspchannel?ch="+String(channel)+"&filters", serverchannel,
                    function(data) {
                        if (modeLocal) data.num = channel;
                        update_channelinfo("editing channel "+channel);
                        if ((dspchannel == null)||(channelmodified == 0)) { // avoid conflicting situation : prioritize client modification
                            dspchannel = data; selfunc = 0;
                            shrinkChannelFilters();
                            DOMrefresh = 1;
                        } else
                            console.log("discard dspchannel received");
                    } ) ; }
        }
    }
    var oldConnect = connected; // used for checking potential transition
    if ((HttpOngoing == 0) && (periodicCount >= pullingStatusDivider)) // if no pending request and time elapsed
        getHttp("/getdspstatus", serverstatus,  // systematic reading of the server status to potentialy update key informations
            function(data) {
                if (oldConnect == 0) {  // reconection or first connection
                    init();             // clear everything
                    dspstatus = data;   // overwrite every thing
                    updateConfigInfo("");
                    console.log("CONNECTED dspstatus = ", dspstatus);
                } else {
                    if (statusmodified == 0) { // avoid conflicting situation : prioritize client modification
                        if ((data.volume != dspstatus.volume) ||
                            (data.source != dspstatus.source) ||
                            (data.config != dspstatus.config)) {
                            dspstatus = data;
                            updateSourceConfigButtonsVolume();
                        } else  dspstatus = data;
                    }
                }
            } );

    if (DOMrefresh) DOM_update();
}




// called when changing the source
function bsource(num){
    if (num != dspstatus.source) {
        dspstatus.source = num;
        statusmodified = 1;
        updateSourceConfigButtonsVolume();
    } else window.location.reload();    // just to test reload :)
}

// called when changing preset / config
function bpreset(num) {
    if (checkModified()) {
        dspstatus.config = num;
        statusmodified = 1;
        channel = 0; selfunc = 0;
        updateSourceConfigButtonsVolume(); }
}

// called when changing or selecting a channel
function bchannel(num){
    channel = num;
    selfunc = 0;
    update_channelinfo("loading channel "+num+"...");
}

// called when changing or selecting a function
function bfunc(num){
    selfunc = num;
    DOM_update();   // also activate the function pannels  if required
    if ((selfunc==selEQ)||(selfunc==selHP)||(selfunc==selLP))  {
        loadFilters();              // load the active filter array 
        updateSelectFilters();      // update the button selector
        getFilters();   // load values of F/Q/G/B/I from array according to selfunc number
        displayFilters();
    } else
    if ( selfunc == selMIXER) {
        updateInputMixers();
        displayMixerLines();
    }
    if ( selfunc == selFIR) {
        // TODO
    }
}


/*
*   function for handling the html element interraction for "filters" functions and panel for EQ/HighPass/LowPass
*
*/

// changing filter's frequency
function finchange(num){
    var x = id("fin"+num);
    var F = parseInt(x.value);
    if (isNaN(F)) F = filters[num-1].F;
    F=checkF(F);
    x.value = F;
    if (F != filters[num-1].F) channelmodified = true;
    filters[num-1].F = F;
}
// changing filter's Q coefficient
function qinchange(num){
    var x = id("qin"+num);
    var Q = parseInt(x.value);
    if (isNaN(Q)) Q = filters[num-1].Q;
    Q=checkQ(Q);
    x.value = Q;
    if (Q != filters[num-1].Q) channelmodified = true;
    filters[num-1].Q = Q;
}
// changing filter's gain
function ginchange(num){
    var x = id("gin"+num);
    var G = parseFloat(x.value);
    if (isNaN(G)) G = filters[num-1].G;
    G=checkG(G);
    if (G != filters[num-1].G) channelmodified = true;
    filters[num-1].G = G;
    x.value = G;
}
// changing filter's invert status
function iinchange(num){
    var x = id("iin"+num);
    channelmodified = true;
    if (x.checked) filters[num-1].I = 1;
    else filters[num-1].I = 0;
}
// changing filter's bypass status
function binchange(num){
    var x = id("bin"+num);
    channelmodified = true;
    if (x.checked) filters[num-1].B = 1;
    else filters[num-1].B = 0;
}

// clicking on a filter line
function  filtclick(num){
    displayFiltersLines();
}

// changing the filter type with the dropdown button
function ftypechange(num){
    var x = id("ftype"+num);
    if (x.value != filters[num-1].T) channelmodified = true;
    filters[num-1].T = x.value;
    displayFilters();
}

/*
*   function for handling the "Mixer" functions and panel
*
*/

function  mixclick(num){
    selmixer = num;
    displayMixerLines();
}


function mixchange(num){
    var x = id("mix"+num);
    var gain = parseFloat(x.value);
    if (isNaN(gain)) gain = mixer[num-1]
    checkG(gain);
    if (gain != mixer[num-1]) configmodified = true;
    mixer[num-1] = gain;
    x.value = gain;
    displayMixerLines();
}

/*
*   function for handling the "Channel" functions and panel
*
*/


function channelfields_update(){
    id("gain").value   = dspconfig.gains[channel-1];
    id("mute").checked = (dspconfig.mutes[channel-1] != 0);
    id("inv").checked  = (dspconfig.inverts[channel-1] != 0);
    id("name").value   = dspconfig.names[channel-1];
    var delay = dspconfig.delays[channel-1];
    if (modedelayus == 0) delay = convertustomm(delay);
    id("delay").value = delay;
    setusmm(modedelayus);
    updateSamples();
}

function gainchange(){
    var x = id("gain");
    var gain = parseFloat(x.value);
    if (isNaN(gain)) gain = dspconfig.gains[channel-1];
    checkG(gain);
    if (gain != dspchannel.gain) configmodified = true;
    dspconfig.gains[channel-1] = gain;
    x.value = gain;
}

function mutechange(){
    var x = id("mute");
    configmodified = true;
    if (x.checked) dspconfig.mutes[channel-1] = 1; else dspconfig.mutes[channel-1] = 0;
}

function invchange(){
    var x = id("inv");
    configmodified = true;
    if (x.checked) dspconfig.inverts[channel-1] = 1; else dspconfig.inverts[channel-1] = 0;
}

function namechange(){
    var x = id("name");
    if (x.value != dspconfig.names[channel-1]) configmodified = true;
    dspconfig.names[channel-1] = x.value;
    DOM_update();
}

function setusmm(modeus){
    var x = id("us"); var y = id("mm");
    if (modeus) x.checked = true; else x.checked = false;
    y.checked = !x.checked;
}
function uschange(){
    var x = id("us");
    if (x.checked) {
        if (modedelayus == 0) { // previously in mm
            id("delay").value = dspconfig.delays[channel-1]; }
        updateSamples();
        modedelayus = 1; }
    setusmm(modedelayus);
    updateSamples();
}

function mmchange(){
    var x = id("mm");
    if (x.checked) {
        if (modedelayus)  // previously in us
            id("delay").value = convertustomm(dspconfig.delays[channel-1]);
        updateSamples();
        modedelayus = 0;  }
    setusmm(modedelayus);
}

function convertustomm(delayus){
    return Math.floor(delayus/1000*celerity);
}
function convertmmtous(distmm){
    return Math.floor(distmm/celerity*1000);
}
function convertustosamples(delayus){
    return Math.floor(delayus*samplingfreq/1000000);
}
function convertsamplestous(samples){
    return Math.floor(1000000/samplingfreq*samples);
}
function updateSamples(){
    var x = id("samples");
    var samples = convertustosamples(dspconfig.delays[channel-1]);
    x.innerHTML = samples+" / "+samplesmax;
}
function delaychange(){
    var x = id("delay");
    var delay = parseFloat(x.value);
    if ((!isNaN(delay))&&(delay>=Dmin)&&(delay<=Dmax)) delay = Math.floor(delay);
    else
    if (modedelayus) delay = dspconfig.delays[channel-1];
    else delay = convertustomm(dspconfig.delays[channel-1]);
    var delayus;
    if (modedelayus) delayus = delay;
    else delayus = convertmmtous(delay);
    var samples = convertustosamples(delayus);
    if (samples>samplesmax) delayus = convertsamplestous(samplesmax);
    if (delayus != dspconfig.delays[channel-1]) configmodified = true;
    dspconfig.delays[channel-1] = delayus;
    if (modedelayus === 0) delay = convertustomm(delayus);
    x.value = delay;
    updateSamples();
}

// volume slider, biggest on the screen, smaller in the code

function sliderchange() {
    var volume = id("webVolume").value;
    if (dspstatus.volume != volume) {
        dspstatus.volume  = volume;
        statusmodified    = 1;
    }
}

function bodyonLoad(){
    if (location.hostname=="") {
        console.log("starting in local mode.")
        modeLocal=1; }
    else console.log("location.hostname=",location.hostname);
    init();
    updateConfigInfo("");
    updateSourceConfigButtonsVolume();
}



/*
    new Chart(id("line-chart"), {
        type: 'line',
        data: {
          labels: [1500,1600,1700,1750,1800,1850,1900,1950,1999,2050],
          datasets: [{ 
              data: [86,114,106,106,107,111,133,221,783,2478],
              label: "Africa",
              borderColor: "#3e95cd",
              fill: false
            }, { 
              data: [282,350,411,502,635,809,947,1402,3700,5267],
              label: "Asia",
              borderColor: "#8e5ea2",
              fill: false
            }, { 
              data: [168,170,178,190,203,276,408,547,675,734],
              label: "Europe",
              borderColor: "#3cba9f",
              fill: false
            }, { 
              data: [40,20,10,16,24,38,74,167,508,784],
              label: "Latin America",
              borderColor: "#e8c3b9",
              fill: false
            }, { 
              data: [6,3,2,2,7,26,82,172,312,433],
              label: "North America",
              borderColor: "#c45850",
              fill: false
            }
          ]
        },
        options: {
          title: {
            display: true,
            text: 'World population per region (in millions)'
          }
        }
      });
*/
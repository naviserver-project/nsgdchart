# If used as a web page
if { [ns_conn isconnected] } { ns_return 200 text/html "Charts generated in /tmp" }

proc samp1 {} {
  set gdc [ns_gdchart create type 3dbar \
                         width 250 \
                         height 230 \
                         bgcolor 0xFFFFFF \
                         linecolor 0x000000 \
                         legend right \
                         legendx 10 \
                         legendy 210 \
                         hardyorig 30]
  ns_gdchart setlabels $gdc { Chicago "New York" "L.A." Atlanta "Paris, MD\n(USA)" London }
  ns_gdchart setdata $gdc d1 { 0.5 0.09 0.6 0.85 0.0 0.90 }
  ns_gdchart setdata $gdc d2 { 1.9 1.3  0.6 0.75 0.1 2.0 }
  ns_gdchart setcolors $gdc { 0x008080 0x8080FF }
  ns_gdchart save $gdc /tmp/gdc_samp1.png
  ns_gdchart destroy $gdc
}

proc samp2 {} {
  set gdc [ns_gdchart create type combohlcarea \
                         width 200 \
                         height 175 \
                         plotcolor 0x000000 \
                         grid 0 \
                         barwidth 75 \
                         title "Widget Corp." \
                         ytitle "Price ($)" \
                         ytitlesize 2 \
                         volcolor 0x4040FF \
                         3ddepth 4.0 \
                         hlccapwidth 45 \
                         hlcstyle "icap closeconnected" \
                         annocolor 0x00FF00 \
                         annotext "Did Not\nTrade" \
                         annopoint 3 \
                         annofontsize 1]
  ns_gdchart setlabels $gdc { May Jun Jul Aug Sep Oct Nov Dec Jan Feb Mar Apr }
  ns_gdchart setdata $gdc d1 { 17.8 17.1 17.3 * 17.2 17.1 17.3 17.3 17.3 17.1 17.5 17.4 }
  ns_gdchart setdata $gdc d2 { 17.0 16.8 16.9 * 16.9 16.8 17.2 16.8 17.0 16.9 16.4 16.1 }
  ns_gdchart setdata $gdc d3 { 16.8 16.8 16.7 * 16.5 16.0 16.1 16.8 16.5 16.9 16.2 16.0 }
  ns_gdchart setdata $gdc d4 { 150.0 100.0 340.0 * 999.0 390.0 420.0 150.0 100.0 340.0 1590.0 700.0 }
  ns_gdchart save $gdc /tmp/gdc_samp2.png
  ns_gdchart destroy $gdc
}

proc samp3 {} {
  set gdc [ns_gdchart create type 3dfloatingbar \
                         width 200 \
                         height 175 \
                         barwidth 60 \
                         xtitle Week \
                         ytitle Data]
  ns_gdchart setlabels $gdc { Sep Oct Nov Dec Jan Feb Mar Apr May Jun }
  ns_gdchart setdata $gdc d1 { 17.0 16.8 16.9 16.9 16.8 17.2 16.8 17.0 16.9 15.8 }
  ns_gdchart setdata $gdc d2 { 16.8 16.8 16.7 16.5 26.0 26.1 26.8 26.5 26.9 24.9 }
  ns_gdchart set $gdc randomextcolors 1
  ns_gdchart save $gdc /tmp/gdc_samp3.png
  ns_gdchart destroy $gdc
}

proc samp4 {} {
  set gdc [ns_gdchart create type 3darea \
                         width 200 \
                         height 175 \
                         xtitle Week \
                         ytitle Data]
  ns_gdchart setlabels $gdc { Sep Oct Nov Dec Jan Feb Mar Apr May Jun }
  ns_gdchart setdata $gdc d1 { 17.0 16.8 16.9 16.9 16.8 17.2 16.8 17.0 16.9 15.8 }
  ns_gdchart setdata $gdc d2 { 16.8 16.8 16.7 16.5 26.0 26.1 26.8 26.5 26.9 24.9 }
  ns_gdchart set $gdc randomextcolors 1
  ns_gdchart save $gdc /tmp/gdc_samp4.png
  ns_gdchart destroy $gdc
}

proc samp5 {} {
  set gdc [ns_gdchart create type combolinearea \
                         width 200 \
                         height 175 \
                         plotcolor 0x000000 \
                         grid 0 \
                         barwidth 75 \
                         title "Widget Corp." \
                         ytitle "Price ($)" \
                         ytitlesize 2 \
                         volcolor 0x4040FF \
                         ylabelfmt %.2f]
  ns_gdchart setlabels $gdc { May Jun Jul Aug Sep Oct Nov Dec Jan Feb Mar }
  ns_gdchart setdata $gdc d1 { 17.8 17.1 17.3 17.2 17.1 17.3 17.3 17.3 17.1 17.5 17.4 }
  ns_gdchart setdata $gdc d2 { 17.0 16.8 16.9 16.9 16.8 17.2 16.8 17.0 16.9 16.4 16.1 }
  ns_gdchart setdata $gdc d3 { 16.8 16.8 16.7 16.5 16.0 16.1 16.8 16.5 16.9 16.2 16.0 }
  ns_gdchart setdata $gdc d4 { 150.0 100.0 340.0 999.0 390.0 420.0 150.0 100.0 340.0 1590.0 700.0 }
  ns_gdchart save $gdc /tmp/gdc_samp5.png
  ns_gdchart destroy $gdc
}

proc samp6 {} {
  set gdc [ns_gdchart create type 3darea \
                         width 250 \
                         height 250 \
                         bgcolor 0xFFFFFF \
                         linecolor 0x000000 \
                         legend bottom \
                         legendx 50 \
                         legendy 230 \
                         hardheight 150]
  ns_gdchart setlabels $gdc { Chicago "New York" "L.A." Atlanta "Paris, MD\n(USA)" London }
  ns_gdchart setdata $gdc d1 { 0.5 0.09 0.6 0.85 0.0 0.90 }
  ns_gdchart setdata $gdc d2 { 1.9 1.3  0.6 0.75 0.1 2.0 }
  ns_gdchart setcolors $gdc { 0x008080 0x8080FF }
  ns_gdchart save $gdc /tmp/gdc_samp6.png
  ns_gdchart destroy $gdc
}

proc samp7 {} {
  set gdc [ns_gdchart create type linearea \
                         width 250 \
                         height 250 \
                         gridontop 1 \
                         bgcolor 0xFFFFFF \
                         gridcolor 0x98a6e0 \
                         linecolor 0x000000 \
                         legend bottom \
                         legendx 50 \
                         legendy 230 \
                         hardheight 150]
  ns_gdchart setlabels $gdc { Chicago "New York" "L.A." Atlanta "Paris, MD\n(USA)" London }
  ns_gdchart setdata $gdc d1 { 0.5 0.09 0.6 0.85 0.0 0.90 }
  ns_gdchart setdata $gdc d2 { 1.9 1.3  0.6 0.75 0.1 2.0 }
  ns_gdchart setdata $gdc d3 { 2.9 2.3  1.6 1.75 2.1 1.0 }
  ns_gdchart setcolors $gdc { 0x008080 0x8080FF 0x4040FF }
  ns_gdchart save $gdc /tmp/gdc_samp7.png
  ns_gdchart destroy $gdc
}

samp1
samp2
samp3
samp4
samp5
samp6
samp7


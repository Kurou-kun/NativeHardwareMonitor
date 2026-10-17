[Variables]

;   Shared by every skin in this config. Rainmeter resolves #@# to this folder, so a
;   skin pulls the lot in with one line:
;
;       [Variables]
;       @Include_Styles="#@#Styles.inc"
;
;   Put the include LAST in a skin's [Variables]. It reopens [Variables] and then opens
;   style sections of its own, so anything written after it in the skin would land in
;   [Style_Wide] rather than in [Variables].
;
;   Everything here is a default, not a rule: a skin that sets scale=2 or its own
;   FontFace after the include wins, because the later definition is the one Rainmeter
;   keeps.

scale=1

;   Colors
color_background=10, 10, 10, 225
color_background_alt=25, 25, 25, 225
color_foreground=240, 240, 240, 255
color_foreground_alt=180, 180, 180, 255
color_accent=240, 0, 0, 225

;   Fonts. Consolas ships with Windows; swap in anything you like.
font_family="Consolas"
font_size_s=10
font_size_m=12
font_size_l=14


;   STYLES
[Style_Header]
FontFace=#font_family#
FontColor=#color_foreground#
FontSize=(#font_size_l# * #scale#)
StringAlign="Center"
StringCase="Upper"
StringStyle="Bold"
InlineSetting=CharacterSpacing | 1 | 1
ClipString=1
AntiAlias=1

[Style_Group]
FontFace=#font_family#
FontColor=#color_foreground_alt#
FontSize=(#font_size_s# * #scale#)
StringAlign="LeftCenter"
StringCase="Upper"
InlineSetting=CharacterSpacing | 1 | 1
ClipString=1
AntiAlias=1

[Style_Label]
FontFace=#font_family#
FontColor=#color_foreground#
FontSize=(#font_size_m# * #scale#)
StringAlign="Left"
ClipString=1
AntiAlias=1

[Style_Value]
FontFace=#font_family#
FontColor=#color_foreground#
FontSize=(#font_size_m# * #scale#)
StringAlign="Right"
ClipString=1
AntiAlias=1

;   Long strings - a GPU name, an adapter description - on their own full-width line
;   under the label rather than squeezed into the right-hand column.
[Style_Wide]
FontFace=#font_family#
FontColor=#color_foreground_alt#
FontSize=(#font_size_m# * #scale#)
StringAlign="Left"
ClipString=1
AntiAlias=1

;   Bars, used by overview.ini. Kept here so a skin copying from it gets them too.
[Style_Bar_Background]
Shape=Rectangle 0, 0, [#CURRENTSECTION#:W], [#CURRENTSECTION#:H], 0 | Fill Color #color_background_alt# | StrokeWidth 0
W=([Background:W] - (10 * #scale#))
H=(4 * #scale#)
X=(5 * #scale#)
Y=(10 * #scale#)R
AntiAlias=1

[Style_Bar]
BarColor=#color_accent#
BarOrientation="Horizontal"
H=(4 * #scale#)
W=([Background:W] - (10 * #scale#))
X=(5 * #scale#)
Y=0r
AntiAlias=1

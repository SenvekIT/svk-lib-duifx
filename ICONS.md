# DUIFx Icon Reference

## Usage

Icons are embedded via the Qt Resource System. Use the resource path `:icons/icon/<Name>.svg`.

### Example: MenuButton (Header)

```python
MenuButton(
    icon_path=":icons/icon/Settings.svg",
    label="Settings",
)
```

### Example: MenuButton (Sidebar)

```python
MenuButton(
    icon_path=":icons/icon/Home.svg",
    tooltip="Home",
)
```

### Example: QIcon

```python
from PySide6.QtGui import QIcon

icon = QIcon(":icons/icon/Home.svg")
button.setIcon(icon)
```

### Example: QPushButton with icon

```python
from PySide6.QtWidgets import QPushButton
from PySide6.QtGui import QIcon

btn = QPushButton()
btn.setIcon(QIcon(":icons/icon/Save.svg"))
btn.setToolTip("Save")
```

### Compile Qt Resource File

After adding or removing icons, recompile the Qt resource file:

```
pyside6-rcc -o resources_rc.py resources.qrc
```

Example with full paths:

```
pyside6-rcc -o .\duifx\gui\ui\resource_rc.py .\duifx\gui\resources\resource.qrc
```

---

## Available Icons

325 icons. Most come from the MIT-licensed
[Flat Color Icons](https://github.com/icons8/flat-color-icons) set — see
[THIRD-PARTY-LICENSES.md](THIRD-PARTY-LICENSES.md). Third-party brand and
product logos from that set have been removed: the artwork is MIT, but the
trademarks it depicts are not Icons8’s to license on.

| Icon | Name | Resource Path |
|------|------|---------------|
| ![about](duifx/gui/resources/icon/about.svg) | about | `:icons/icon/about.svg` |
| ![accept_database](duifx/gui/resources/icon/accept_database.svg) | accept_database | `:icons/icon/accept_database.svg` |
| ![add_column](duifx/gui/resources/icon/add_column.svg) | add_column | `:icons/icon/add_column.svg` |
| ![add_database](duifx/gui/resources/icon/add_database.svg) | add_database | `:icons/icon/add_database.svg` |
| ![add_image](duifx/gui/resources/icon/add_image.svg) | add_image | `:icons/icon/add_image.svg` |
| ![add_row](duifx/gui/resources/icon/add_row.svg) | add_row | `:icons/icon/add_row.svg` |
| ![address_book](duifx/gui/resources/icon/address_book.svg) | address_book | `:icons/icon/address_book.svg` |
| ![advance](duifx/gui/resources/icon/advance.svg) | advance | `:icons/icon/advance.svg` |
| ![advertising](duifx/gui/resources/icon/advertising.svg) | advertising | `:icons/icon/advertising.svg` |
| ![alarm_clock](duifx/gui/resources/icon/alarm_clock.svg) | alarm_clock | `:icons/icon/alarm_clock.svg` |
| ![Alert](duifx/gui/resources/icon/Alert.svg) | Alert | `:icons/icon/Alert.svg` |
| ![alphabetical_sorting_az](duifx/gui/resources/icon/alphabetical_sorting_az.svg) | alphabetical_sorting_az | `:icons/icon/alphabetical_sorting_az.svg` |
| ![alphabetical_sorting_za](duifx/gui/resources/icon/alphabetical_sorting_za.svg) | alphabetical_sorting_za | `:icons/icon/alphabetical_sorting_za.svg` |
| ![answers](duifx/gui/resources/icon/answers.svg) | answers | `:icons/icon/answers.svg` |
| ![approval](duifx/gui/resources/icon/approval.svg) | approval | `:icons/icon/approval.svg` |
| ![approve](duifx/gui/resources/icon/approve.svg) | approve | `:icons/icon/approve.svg` |
| ![area_chart](duifx/gui/resources/icon/area_chart.svg) | area_chart | `:icons/icon/area_chart.svg` |
| ![arrow-down-icon](duifx/gui/resources/icon/arrow-down-icon.svg) | arrow-down-icon | `:icons/icon/arrow-down-icon.svg` |
| ![arrow-up-icon](duifx/gui/resources/icon/arrow-up-icon.svg) | arrow-up-icon | `:icons/icon/arrow-up-icon.svg` |
| ![assistant](duifx/gui/resources/icon/assistant.svg) | assistant | `:icons/icon/assistant.svg` |
| ![audio_file](duifx/gui/resources/icon/audio_file.svg) | audio_file | `:icons/icon/audio_file.svg` |
| ![automatic](duifx/gui/resources/icon/automatic.svg) | automatic | `:icons/icon/automatic.svg` |
| ![automotive](duifx/gui/resources/icon/automotive.svg) | automotive | `:icons/icon/automotive.svg` |
| ![bad_decision](duifx/gui/resources/icon/bad_decision.svg) | bad_decision | `:icons/icon/bad_decision.svg` |
| ![bar_chart](duifx/gui/resources/icon/bar_chart.svg) | bar_chart | `:icons/icon/bar_chart.svg` |
| ![bearish](duifx/gui/resources/icon/bearish.svg) | bearish | `:icons/icon/bearish.svg` |
| ![binoculars](duifx/gui/resources/icon/binoculars.svg) | binoculars | `:icons/icon/binoculars.svg` |
| ![biohazard](duifx/gui/resources/icon/biohazard.svg) | biohazard | `:icons/icon/biohazard.svg` |
| ![biomass](duifx/gui/resources/icon/biomass.svg) | biomass | `:icons/icon/biomass.svg` |
| ![biotech](duifx/gui/resources/icon/biotech.svg) | biotech | `:icons/icon/biotech.svg` |
| ![bookmark](duifx/gui/resources/icon/bookmark.svg) | bookmark | `:icons/icon/bookmark.svg` |
| ![briefcase](duifx/gui/resources/icon/briefcase.svg) | briefcase | `:icons/icon/briefcase.svg` |
| ![broken_link](duifx/gui/resources/icon/broken_link.svg) | broken_link | `:icons/icon/broken_link.svg` |
| ![bullish](duifx/gui/resources/icon/bullish.svg) | bullish | `:icons/icon/bullish.svg` |
| ![business](duifx/gui/resources/icon/business.svg) | business | `:icons/icon/business.svg` |
| ![business_contact](duifx/gui/resources/icon/business_contact.svg) | business_contact | `:icons/icon/business_contact.svg` |
| ![businessman](duifx/gui/resources/icon/businessman.svg) | businessman | `:icons/icon/businessman.svg` |
| ![businesswoman](duifx/gui/resources/icon/businesswoman.svg) | businesswoman | `:icons/icon/businesswoman.svg` |
| ![butting_in](duifx/gui/resources/icon/butting_in.svg) | butting_in | `:icons/icon/butting_in.svg` |
| ![cable_release](duifx/gui/resources/icon/cable_release.svg) | cable_release | `:icons/icon/cable_release.svg` |
| ![calculator](duifx/gui/resources/icon/calculator.svg) | calculator | `:icons/icon/calculator.svg` |
| ![calendar](duifx/gui/resources/icon/calendar.svg) | calendar | `:icons/icon/calendar.svg` |
| ![call_transfer](duifx/gui/resources/icon/call_transfer.svg) | call_transfer | `:icons/icon/call_transfer.svg` |
| ![callback](duifx/gui/resources/icon/callback.svg) | callback | `:icons/icon/callback.svg` |
| ![camcorder](duifx/gui/resources/icon/camcorder.svg) | camcorder | `:icons/icon/camcorder.svg` |
| ![camcorder_pro](duifx/gui/resources/icon/camcorder_pro.svg) | camcorder_pro | `:icons/icon/camcorder_pro.svg` |
| ![camera](duifx/gui/resources/icon/camera.svg) | camera | `:icons/icon/camera.svg` |
| ![camera_addon](duifx/gui/resources/icon/camera_addon.svg) | camera_addon | `:icons/icon/camera_addon.svg` |
| ![camera_identification](duifx/gui/resources/icon/camera_identification.svg) | camera_identification | `:icons/icon/camera_identification.svg` |
| ![cancel](duifx/gui/resources/icon/cancel.svg) | cancel | `:icons/icon/cancel.svg` |
| ![candle_sticks](duifx/gui/resources/icon/candle_sticks.svg) | candle_sticks | `:icons/icon/candle_sticks.svg` |
| ![capacitor](duifx/gui/resources/icon/capacitor.svg) | capacitor | `:icons/icon/capacitor.svg` |
| ![cell_phone](duifx/gui/resources/icon/cell_phone.svg) | cell_phone | `:icons/icon/cell_phone.svg` |
| ![Channels](duifx/gui/resources/icon/Channels.svg) | Channels | `:icons/icon/Channels.svg` |
| ![charge_battery](duifx/gui/resources/icon/charge_battery.svg) | charge_battery | `:icons/icon/charge_battery.svg` |
| ![checkmark](duifx/gui/resources/icon/checkmark.svg) | checkmark | `:icons/icon/checkmark.svg` |
| ![circuit](duifx/gui/resources/icon/circuit.svg) | circuit | `:icons/icon/circuit.svg` |
| ![clapperboard](duifx/gui/resources/icon/clapperboard.svg) | clapperboard | `:icons/icon/clapperboard.svg` |
| ![clear_filters](duifx/gui/resources/icon/clear_filters.svg) | clear_filters | `:icons/icon/clear_filters.svg` |
| ![clock](duifx/gui/resources/icon/clock.svg) | clock | `:icons/icon/clock.svg` |
| ![close_up_mode](duifx/gui/resources/icon/close_up_mode.svg) | close_up_mode | `:icons/icon/close_up_mode.svg` |
| ![cloth](duifx/gui/resources/icon/cloth.svg) | cloth | `:icons/icon/cloth.svg` |
| ![collaboration](duifx/gui/resources/icon/collaboration.svg) | collaboration | `:icons/icon/collaboration.svg` |
| ![collapse](duifx/gui/resources/icon/collapse.svg) | collapse | `:icons/icon/collapse.svg` |
| ![collect](duifx/gui/resources/icon/collect.svg) | collect | `:icons/icon/collect.svg` |
| ![combo_chart](duifx/gui/resources/icon/combo_chart.svg) | combo_chart | `:icons/icon/combo_chart.svg` |
| ![command_line](duifx/gui/resources/icon/command_line.svg) | command_line | `:icons/icon/command_line.svg` |
| ![comments](duifx/gui/resources/icon/comments.svg) | comments | `:icons/icon/comments.svg` |
| ![compact_camera](duifx/gui/resources/icon/compact_camera.svg) | compact_camera | `:icons/icon/compact_camera.svg` |
| ![conference_call](duifx/gui/resources/icon/conference_call.svg) | conference_call | `:icons/icon/conference_call.svg` |
| ![contacts](duifx/gui/resources/icon/contacts.svg) | contacts | `:icons/icon/contacts.svg` |
| ![copyleft](duifx/gui/resources/icon/copyleft.svg) | copyleft | `:icons/icon/copyleft.svg` |
| ![copyright](duifx/gui/resources/icon/copyright.svg) | copyright | `:icons/icon/copyright.svg` |
| ![crystal_oscillator](duifx/gui/resources/icon/crystal_oscillator.svg) | crystal_oscillator | `:icons/icon/crystal_oscillator.svg` |
| ![currency_exchange](duifx/gui/resources/icon/currency_exchange.svg) | currency_exchange | `:icons/icon/currency_exchange.svg` |
| ![cursor](duifx/gui/resources/icon/cursor.svg) | cursor | `:icons/icon/cursor.svg` |
| ![customer_support](duifx/gui/resources/icon/customer_support.svg) | customer_support | `:icons/icon/customer_support.svg` |
| ![dam](duifx/gui/resources/icon/dam.svg) | dam | `:icons/icon/dam.svg` |
| ![data_backup](duifx/gui/resources/icon/data_backup.svg) | data_backup | `:icons/icon/data_backup.svg` |
| ![data_configuration](duifx/gui/resources/icon/data_configuration.svg) | data_configuration | `:icons/icon/data_configuration.svg` |
| ![data_encryption](duifx/gui/resources/icon/data_encryption.svg) | data_encryption | `:icons/icon/data_encryption.svg` |
| ![data_protection](duifx/gui/resources/icon/data_protection.svg) | data_protection | `:icons/icon/data_protection.svg` |
| ![data_recovery](duifx/gui/resources/icon/data_recovery.svg) | data_recovery | `:icons/icon/data_recovery.svg` |
| ![data_sheet](duifx/gui/resources/icon/data_sheet.svg) | data_sheet | `:icons/icon/data_sheet.svg` |
| ![database](duifx/gui/resources/icon/database.svg) | database | `:icons/icon/database.svg` |
| ![debt](duifx/gui/resources/icon/debt.svg) | debt | `:icons/icon/debt.svg` |
| ![decision](duifx/gui/resources/icon/decision.svg) | decision | `:icons/icon/decision.svg` |
| ![delete_column](duifx/gui/resources/icon/delete_column.svg) | delete_column | `:icons/icon/delete_column.svg` |
| ![delete_database](duifx/gui/resources/icon/delete_database.svg) | delete_database | `:icons/icon/delete_database.svg` |
| ![delete_row](duifx/gui/resources/icon/delete_row.svg) | delete_row | `:icons/icon/delete_row.svg` |
| ![department](duifx/gui/resources/icon/department.svg) | department | `:icons/icon/department.svg` |
| ![deployment](duifx/gui/resources/icon/deployment.svg) | deployment | `:icons/icon/deployment.svg` |
| ![diploma_1](duifx/gui/resources/icon/diploma_1.svg) | diploma_1 | `:icons/icon/diploma_1.svg` |
| ![diploma_2](duifx/gui/resources/icon/diploma_2.svg) | diploma_2 | `:icons/icon/diploma_2.svg` |
| ![disapprove](duifx/gui/resources/icon/disapprove.svg) | disapprove | `:icons/icon/disapprove.svg` |
| ![disclaimer](duifx/gui/resources/icon/disclaimer.svg) | disclaimer | `:icons/icon/disclaimer.svg` |
| ![dislike](duifx/gui/resources/icon/dislike.svg) | dislike | `:icons/icon/dislike.svg` |
| ![display](duifx/gui/resources/icon/display.svg) | display | `:icons/icon/display.svg` |
| ![do_not_inhale](duifx/gui/resources/icon/do_not_inhale.svg) | do_not_inhale | `:icons/icon/do_not_inhale.svg` |
| ![do_not_insert](duifx/gui/resources/icon/do_not_insert.svg) | do_not_insert | `:icons/icon/do_not_insert.svg` |
| ![do_not_mix](duifx/gui/resources/icon/do_not_mix.svg) | do_not_mix | `:icons/icon/do_not_mix.svg` |
| ![document](duifx/gui/resources/icon/document.svg) | document | `:icons/icon/document.svg` |
| ![donate](duifx/gui/resources/icon/donate.svg) | donate | `:icons/icon/donate.svg` |
| ![doughnut_chart](duifx/gui/resources/icon/doughnut_chart.svg) | doughnut_chart | `:icons/icon/doughnut_chart.svg` |
| ![down](duifx/gui/resources/icon/down.svg) | down | `:icons/icon/down.svg` |
| ![down_left](duifx/gui/resources/icon/down_left.svg) | down_left | `:icons/icon/down_left.svg` |
| ![down_right](duifx/gui/resources/icon/down_right.svg) | down_right | `:icons/icon/down_right.svg` |
| ![download](duifx/gui/resources/icon/download.svg) | download | `:icons/icon/download.svg` |
| ![Dx](duifx/gui/resources/icon/Dx.svg) | Dx | `:icons/icon/Dx.svg` |
| ![edit_image](duifx/gui/resources/icon/edit_image.svg) | edit_image | `:icons/icon/edit_image.svg` |
| ![electrical_sensor](duifx/gui/resources/icon/electrical_sensor.svg) | electrical_sensor | `:icons/icon/electrical_sensor.svg` |
| ![electrical_threshold](duifx/gui/resources/icon/electrical_threshold.svg) | electrical_threshold | `:icons/icon/electrical_threshold.svg` |
| ![electricity](duifx/gui/resources/icon/electricity.svg) | electricity | `:icons/icon/electricity.svg` |
| ![electro_devices](duifx/gui/resources/icon/electro_devices.svg) | electro_devices | `:icons/icon/electro_devices.svg` |
| ![electronics](duifx/gui/resources/icon/electronics.svg) | electronics | `:icons/icon/electronics.svg` |
| ![empty_battery](duifx/gui/resources/icon/empty_battery.svg) | empty_battery | `:icons/icon/empty_battery.svg` |
| ![empty_filter](duifx/gui/resources/icon/empty_filter.svg) | empty_filter | `:icons/icon/empty_filter.svg` |
| ![empty_trash](duifx/gui/resources/icon/empty_trash.svg) | empty_trash | `:icons/icon/empty_trash.svg` |
| ![end_call](duifx/gui/resources/icon/end_call.svg) | end_call | `:icons/icon/end_call.svg` |
| ![engineering](duifx/gui/resources/icon/engineering.svg) | engineering | `:icons/icon/engineering.svg` |
| ![entering_heaven_alive](duifx/gui/resources/icon/entering_heaven_alive.svg) | entering_heaven_alive | `:icons/icon/entering_heaven_alive.svg` |
| ![expand](duifx/gui/resources/icon/expand.svg) | expand | `:icons/icon/expand.svg` |
| ![expired](duifx/gui/resources/icon/expired.svg) | expired | `:icons/icon/expired.svg` |
| ![export](duifx/gui/resources/icon/export.svg) | export | `:icons/icon/export.svg` |
| ![external](duifx/gui/resources/icon/external.svg) | external | `:icons/icon/external.svg` |
| ![factory](duifx/gui/resources/icon/factory.svg) | factory | `:icons/icon/factory.svg` |
| ![factory_breakdown](duifx/gui/resources/icon/factory_breakdown.svg) | factory_breakdown | `:icons/icon/factory_breakdown.svg` |
| ![faq](duifx/gui/resources/icon/faq.svg) | faq | `:icons/icon/faq.svg` |
| ![feed_in](duifx/gui/resources/icon/feed_in.svg) | feed_in | `:icons/icon/feed_in.svg` |
| ![feedback](duifx/gui/resources/icon/feedback.svg) | feedback | `:icons/icon/feedback.svg` |
| ![File Error](duifx/gui/resources/icon/File%20Error.svg) | File Error | `:icons/icon/File Error.svg` |
| ![file](duifx/gui/resources/icon/file.svg) | file | `:icons/icon/file.svg` |
| ![filing_cabinet](duifx/gui/resources/icon/filing_cabinet.svg) | filing_cabinet | `:icons/icon/filing_cabinet.svg` |
| ![filled_filter](duifx/gui/resources/icon/filled_filter.svg) | filled_filter | `:icons/icon/filled_filter.svg` |
| ![film](duifx/gui/resources/icon/film.svg) | film | `:icons/icon/film.svg` |
| ![film_reel](duifx/gui/resources/icon/film_reel.svg) | film_reel | `:icons/icon/film_reel.svg` |
| ![fine_print](duifx/gui/resources/icon/fine_print.svg) | fine_print | `:icons/icon/fine_print.svg` |
| ![flash_auto](duifx/gui/resources/icon/flash_auto.svg) | flash_auto | `:icons/icon/flash_auto.svg` |
| ![flash_off](duifx/gui/resources/icon/flash_off.svg) | flash_off | `:icons/icon/flash_off.svg` |
| ![flash_on](duifx/gui/resources/icon/flash_on.svg) | flash_on | `:icons/icon/flash_on.svg` |
| ![flow_chart](duifx/gui/resources/icon/flow_chart.svg) | flow_chart | `:icons/icon/flow_chart.svg` |
| ![folder](duifx/gui/resources/icon/folder.svg) | folder | `:icons/icon/folder.svg` |
| ![frame](duifx/gui/resources/icon/frame.svg) | frame | `:icons/icon/frame.svg` |
| ![full_battery](duifx/gui/resources/icon/full_battery.svg) | full_battery | `:icons/icon/full_battery.svg` |
| ![full_trash](duifx/gui/resources/icon/full_trash.svg) | full_trash | `:icons/icon/full_trash.svg` |
| ![gallery](duifx/gui/resources/icon/gallery.svg) | gallery | `:icons/icon/gallery.svg` |
| ![genealogy](duifx/gui/resources/icon/genealogy.svg) | genealogy | `:icons/icon/genealogy.svg` |
| ![generic_sorting_asc](duifx/gui/resources/icon/generic_sorting_asc.svg) | generic_sorting_asc | `:icons/icon/generic_sorting_asc.svg` |
| ![generic_sorting_desc](duifx/gui/resources/icon/generic_sorting_desc.svg) | generic_sorting_desc | `:icons/icon/generic_sorting_desc.svg` |
| ![globe](duifx/gui/resources/icon/globe.svg) | globe | `:icons/icon/globe.svg` |
| ![good_decision](duifx/gui/resources/icon/good_decision.svg) | good_decision | `:icons/icon/good_decision.svg` |
| ![graduation_cap](duifx/gui/resources/icon/graduation_cap.svg) | graduation_cap | `:icons/icon/graduation_cap.svg` |
| ![grid](duifx/gui/resources/icon/grid.svg) | grid | `:icons/icon/grid.svg` |
| ![headset](duifx/gui/resources/icon/headset.svg) | headset | `:icons/icon/headset.svg` |
| ![heat_map](duifx/gui/resources/icon/heat_map.svg) | heat_map | `:icons/icon/heat_map.svg` |
| ![high_battery](duifx/gui/resources/icon/high_battery.svg) | high_battery | `:icons/icon/high_battery.svg` |
| ![high_priority](duifx/gui/resources/icon/high_priority.svg) | high_priority | `:icons/icon/high_priority.svg` |
| ![home](duifx/gui/resources/icon/home.svg) | home | `:icons/icon/home.svg` |
| ![idea](duifx/gui/resources/icon/idea.svg) | idea | `:icons/icon/idea.svg` |
| ![image_file](duifx/gui/resources/icon/image_file.svg) | image_file | `:icons/icon/image_file.svg` |
| ![import](duifx/gui/resources/icon/import.svg) | import | `:icons/icon/import.svg` |
| ![in_transit](duifx/gui/resources/icon/in_transit.svg) | in_transit | `:icons/icon/in_transit.svg` |
| ![info](duifx/gui/resources/icon/info.svg) | info | `:icons/icon/info.svg` |
| ![inspection](duifx/gui/resources/icon/inspection.svg) | inspection | `:icons/icon/inspection.svg` |
| ![integrated_webcam](duifx/gui/resources/icon/integrated_webcam.svg) | integrated_webcam | `:icons/icon/integrated_webcam.svg` |
| ![internal](duifx/gui/resources/icon/internal.svg) | internal | `:icons/icon/internal.svg` |
| ![invite](duifx/gui/resources/icon/invite.svg) | invite | `:icons/icon/invite.svg` |
| ![key](duifx/gui/resources/icon/key.svg) | key | `:icons/icon/key.svg` |
| ![landscape](duifx/gui/resources/icon/landscape.svg) | landscape | `:icons/icon/landscape.svg` |
| ![leave](duifx/gui/resources/icon/leave.svg) | leave | `:icons/icon/leave.svg` |
| ![left](duifx/gui/resources/icon/left.svg) | left | `:icons/icon/left.svg` |
| ![left_down](duifx/gui/resources/icon/left_down.svg) | left_down | `:icons/icon/left_down.svg` |
| ![left_down2](duifx/gui/resources/icon/left_down2.svg) | left_down2 | `:icons/icon/left_down2.svg` |
| ![left_up](duifx/gui/resources/icon/left_up.svg) | left_up | `:icons/icon/left_up.svg` |
| ![left_up2](duifx/gui/resources/icon/left_up2.svg) | left_up2 | `:icons/icon/left_up2.svg` |
| ![library](duifx/gui/resources/icon/library.svg) | library | `:icons/icon/library.svg` |
| ![light_at_the_end_of_tunnel](duifx/gui/resources/icon/light_at_the_end_of_tunnel.svg) | light_at_the_end_of_tunnel | `:icons/icon/light_at_the_end_of_tunnel.svg` |
| ![like](duifx/gui/resources/icon/like.svg) | like | `:icons/icon/like.svg` |
| ![like_placeholder](duifx/gui/resources/icon/like_placeholder.svg) | like_placeholder | `:icons/icon/like_placeholder.svg` |
| ![line_chart](duifx/gui/resources/icon/line_chart.svg) | line_chart | `:icons/icon/line_chart.svg` |
| ![link](duifx/gui/resources/icon/link.svg) | link | `:icons/icon/link.svg` |
| ![list](duifx/gui/resources/icon/list.svg) | list | `:icons/icon/list.svg` |
| ![lock](duifx/gui/resources/icon/lock.svg) | lock | `:icons/icon/lock.svg` |
| ![lock_landscape](duifx/gui/resources/icon/lock_landscape.svg) | lock_landscape | `:icons/icon/lock_landscape.svg` |
| ![lock_portrait](duifx/gui/resources/icon/lock_portrait.svg) | lock_portrait | `:icons/icon/lock_portrait.svg` |
| ![low_battery](duifx/gui/resources/icon/low_battery.svg) | low_battery | `:icons/icon/low_battery.svg` |
| ![low_priority](duifx/gui/resources/icon/low_priority.svg) | low_priority | `:icons/icon/low_priority.svg` |
| ![make_decision](duifx/gui/resources/icon/make_decision.svg) | make_decision | `:icons/icon/make_decision.svg` |
| ![manager](duifx/gui/resources/icon/manager.svg) | manager | `:icons/icon/manager.svg` |
| ![Master](duifx/gui/resources/icon/Master.svg) | Master | `:icons/icon/Master.svg` |
| ![medium_priority](duifx/gui/resources/icon/medium_priority.svg) | medium_priority | `:icons/icon/medium_priority.svg` |
| ![menu](duifx/gui/resources/icon/menu.svg) | menu | `:icons/icon/menu.svg` |
| ![middle_battery](duifx/gui/resources/icon/middle_battery.svg) | middle_battery | `:icons/icon/middle_battery.svg` |
| ![mind_map](duifx/gui/resources/icon/mind_map.svg) | mind_map | `:icons/icon/mind_map.svg` |
| ![minus](duifx/gui/resources/icon/minus.svg) | minus | `:icons/icon/minus.svg` |
| ![minus_bw](duifx/gui/resources/icon/minus_bw.svg) | minus_bw | `:icons/icon/minus_bw.svg` |
| ![missed_call](duifx/gui/resources/icon/missed_call.svg) | missed_call | `:icons/icon/missed_call.svg` |
| ![mms](duifx/gui/resources/icon/mms.svg) | mms | `:icons/icon/mms.svg` |
| ![money_transfer](duifx/gui/resources/icon/money_transfer.svg) | money_transfer | `:icons/icon/money_transfer.svg` |
| ![multiple_cameras](duifx/gui/resources/icon/multiple_cameras.svg) | multiple_cameras | `:icons/icon/multiple_cameras.svg` |
| ![multiple_devices](duifx/gui/resources/icon/multiple_devices.svg) | multiple_devices | `:icons/icon/multiple_devices.svg` |
| ![multiple_inputs](duifx/gui/resources/icon/multiple_inputs.svg) | multiple_inputs | `:icons/icon/multiple_inputs.svg` |
| ![multiple_smartphones](duifx/gui/resources/icon/multiple_smartphones.svg) | multiple_smartphones | `:icons/icon/multiple_smartphones.svg` |
| ![music](duifx/gui/resources/icon/music.svg) | music | `:icons/icon/music.svg` |
| ![negative_dynamic](duifx/gui/resources/icon/negative_dynamic.svg) | negative_dynamic | `:icons/icon/negative_dynamic.svg` |
| ![neutral_decision](duifx/gui/resources/icon/neutral_decision.svg) | neutral_decision | `:icons/icon/neutral_decision.svg` |
| ![neutral_trading](duifx/gui/resources/icon/neutral_trading.svg) | neutral_trading | `:icons/icon/neutral_trading.svg` |
| ![news](duifx/gui/resources/icon/news.svg) | news | `:icons/icon/news.svg` |
| ![next](duifx/gui/resources/icon/next.svg) | next | `:icons/icon/next.svg` |
| ![nfc_sign](duifx/gui/resources/icon/nfc_sign.svg) | nfc_sign | `:icons/icon/nfc_sign.svg` |
| ![night_landscape](duifx/gui/resources/icon/night_landscape.svg) | night_landscape | `:icons/icon/night_landscape.svg` |
| ![night_portrait](duifx/gui/resources/icon/night_portrait.svg) | night_portrait | `:icons/icon/night_portrait.svg` |
| ![no_idea](duifx/gui/resources/icon/no_idea.svg) | no_idea | `:icons/icon/no_idea.svg` |
| ![no_video](duifx/gui/resources/icon/no_video.svg) | no_video | `:icons/icon/no_video.svg` |
| ![numerical_sorting_12](duifx/gui/resources/icon/numerical_sorting_12.svg) | numerical_sorting_12 | `:icons/icon/numerical_sorting_12.svg` |
| ![numerical_sorting_21](duifx/gui/resources/icon/numerical_sorting_21.svg) | numerical_sorting_21 | `:icons/icon/numerical_sorting_21.svg` |
| ![object_directory](duifx/gui/resources/icon/object_directory.svg) | object_directory | `:icons/icon/object_directory.svg` |
| ![ok](duifx/gui/resources/icon/ok.svg) | ok | `:icons/icon/ok.svg` |
| ![old_time_camera](duifx/gui/resources/icon/old_time_camera.svg) | old_time_camera | `:icons/icon/old_time_camera.svg` |
| ![online_support](duifx/gui/resources/icon/online_support.svg) | online_support | `:icons/icon/online_support.svg` |
| ![opened_folder](duifx/gui/resources/icon/opened_folder.svg) | opened_folder | `:icons/icon/opened_folder.svg` |
| ![org_unit](duifx/gui/resources/icon/org_unit.svg) | org_unit | `:icons/icon/org_unit.svg` |
| ![organization](duifx/gui/resources/icon/organization.svg) | organization | `:icons/icon/organization.svg` |
| ![overtime](duifx/gui/resources/icon/overtime.svg) | overtime | `:icons/icon/overtime.svg` |
| ![package](duifx/gui/resources/icon/package.svg) | package | `:icons/icon/package.svg` |
| ![paid](duifx/gui/resources/icon/paid.svg) | paid | `:icons/icon/paid.svg` |
| ![panorama](duifx/gui/resources/icon/panorama.svg) | panorama | `:icons/icon/panorama.svg` |
| ![parallel_tasks](duifx/gui/resources/icon/parallel_tasks.svg) | parallel_tasks | `:icons/icon/parallel_tasks.svg` |
| ![phone](duifx/gui/resources/icon/phone.svg) | phone | `:icons/icon/phone.svg` |
| ![phone_android](duifx/gui/resources/icon/phone_android.svg) | phone_android | `:icons/icon/phone_android.svg` |
| ![photo_reel](duifx/gui/resources/icon/photo_reel.svg) | photo_reel | `:icons/icon/photo_reel.svg` |
| ![picture](duifx/gui/resources/icon/picture.svg) | picture | `:icons/icon/picture.svg` |
| ![pie_chart](duifx/gui/resources/icon/pie_chart.svg) | pie_chart | `:icons/icon/pie_chart.svg` |
| ![planner](duifx/gui/resources/icon/planner.svg) | planner | `:icons/icon/planner.svg` |
| ![plus](duifx/gui/resources/icon/plus.svg) | plus | `:icons/icon/plus.svg` |
| ![plus_bw](duifx/gui/resources/icon/plus_bw.svg) | plus_bw | `:icons/icon/plus_bw.svg` |
| ![podium_with_audience](duifx/gui/resources/icon/podium_with_audience.svg) | podium_with_audience | `:icons/icon/podium_with_audience.svg` |
| ![podium_with_speaker](duifx/gui/resources/icon/podium_with_speaker.svg) | podium_with_speaker | `:icons/icon/podium_with_speaker.svg` |
| ![podium_without_speaker](duifx/gui/resources/icon/podium_without_speaker.svg) | podium_without_speaker | `:icons/icon/podium_without_speaker.svg` |
| ![portrait_mode](duifx/gui/resources/icon/portrait_mode.svg) | portrait_mode | `:icons/icon/portrait_mode.svg` |
| ![positive_dynamic](duifx/gui/resources/icon/positive_dynamic.svg) | positive_dynamic | `:icons/icon/positive_dynamic.svg` |
| ![previous](duifx/gui/resources/icon/previous.svg) | previous | `:icons/icon/previous.svg` |
| ![print](duifx/gui/resources/icon/print.svg) | print | `:icons/icon/print.svg` |
| ![privacy](duifx/gui/resources/icon/privacy.svg) | privacy | `:icons/icon/privacy.svg` |
| ![process](duifx/gui/resources/icon/process.svg) | process | `:icons/icon/process.svg` |
| ![puzzle](duifx/gui/resources/icon/puzzle.svg) | puzzle | `:icons/icon/puzzle.svg` |
| ![qr](duifx/gui/resources/icon/qr.svg) | qr | `:icons/icon/qr.svg` |
| ![questions](duifx/gui/resources/icon/questions.svg) | questions | `:icons/icon/questions.svg` |
| ![radar_plot](duifx/gui/resources/icon/radar_plot.svg) | radar_plot | `:icons/icon/radar_plot.svg` |
| ![rating](duifx/gui/resources/icon/rating.svg) | rating | `:icons/icon/rating.svg` |
| ![ratings](duifx/gui/resources/icon/ratings.svg) | ratings | `:icons/icon/ratings.svg` |
| ![reading](duifx/gui/resources/icon/reading.svg) | reading | `:icons/icon/reading.svg` |
| ![reading_ebook](duifx/gui/resources/icon/reading_ebook.svg) | reading_ebook | `:icons/icon/reading_ebook.svg` |
| ![record](duifx/gui/resources/icon/record.svg) | record | `:icons/icon/record.svg` |
| ![redo](duifx/gui/resources/icon/redo.svg) | redo | `:icons/icon/redo.svg` |
| ![refresh](duifx/gui/resources/icon/refresh.svg) | refresh | `:icons/icon/refresh.svg` |
| ![registered_trademark](duifx/gui/resources/icon/registered_trademark.svg) | registered_trademark | `:icons/icon/registered_trademark.svg` |
| ![remove_image](duifx/gui/resources/icon/remove_image.svg) | remove_image | `:icons/icon/remove_image.svg` |
| ![reuse](duifx/gui/resources/icon/reuse.svg) | reuse | `:icons/icon/reuse.svg` |
| ![right](duifx/gui/resources/icon/right.svg) | right | `:icons/icon/right.svg` |
| ![right_down](duifx/gui/resources/icon/right_down.svg) | right_down | `:icons/icon/right_down.svg` |
| ![right_down2](duifx/gui/resources/icon/right_down2.svg) | right_down2 | `:icons/icon/right_down2.svg` |
| ![right_up](duifx/gui/resources/icon/right_up.svg) | right_up | `:icons/icon/right_up.svg` |
| ![right_up2](duifx/gui/resources/icon/right_up2.svg) | right_up2 | `:icons/icon/right_up2.svg` |
| ![Risk](duifx/gui/resources/icon/Risk.svg) | Risk | `:icons/icon/Risk.svg` |
| ![rotate_camera](duifx/gui/resources/icon/rotate_camera.svg) | rotate_camera | `:icons/icon/rotate_camera.svg` |
| ![rotate_to_landscape](duifx/gui/resources/icon/rotate_to_landscape.svg) | rotate_to_landscape | `:icons/icon/rotate_to_landscape.svg` |
| ![rotate_to_portrait](duifx/gui/resources/icon/rotate_to_portrait.svg) | rotate_to_portrait | `:icons/icon/rotate_to_portrait.svg` |
| ![ruler](duifx/gui/resources/icon/ruler.svg) | ruler | `:icons/icon/ruler.svg` |
| ![rules](duifx/gui/resources/icon/rules.svg) | rules | `:icons/icon/rules.svg` |
| ![safe](duifx/gui/resources/icon/safe.svg) | safe | `:icons/icon/safe.svg` |
| ![sales_performance](duifx/gui/resources/icon/sales_performance.svg) | sales_performance | `:icons/icon/sales_performance.svg` |
| ![scatter_plot](duifx/gui/resources/icon/scatter_plot.svg) | scatter_plot | `:icons/icon/scatter_plot.svg` |
| ![search](duifx/gui/resources/icon/search.svg) | search | `:icons/icon/search.svg` |
| ![self_service_kiosk](duifx/gui/resources/icon/self_service_kiosk.svg) | self_service_kiosk | `:icons/icon/self_service_kiosk.svg` |
| ![selfie](duifx/gui/resources/icon/selfie.svg) | selfie | `:icons/icon/selfie.svg` |
| ![serial_tasks](duifx/gui/resources/icon/serial_tasks.svg) | serial_tasks | `:icons/icon/serial_tasks.svg` |
| ![service_mark](duifx/gui/resources/icon/service_mark.svg) | service_mark | `:icons/icon/service_mark.svg` |
| ![services](duifx/gui/resources/icon/services.svg) | services | `:icons/icon/services.svg` |
| ![settings](duifx/gui/resources/icon/settings.svg) | settings | `:icons/icon/settings.svg` |
| ![share](duifx/gui/resources/icon/share.svg) | share | `:icons/icon/share.svg` |
| ![shipped](duifx/gui/resources/icon/shipped.svg) | shipped | `:icons/icon/shipped.svg` |
| ![shop](duifx/gui/resources/icon/shop.svg) | shop | `:icons/icon/shop.svg` |
| ![signature](duifx/gui/resources/icon/signature.svg) | signature | `:icons/icon/signature.svg` |
| ![sim_card](duifx/gui/resources/icon/sim_card.svg) | sim_card | `:icons/icon/sim_card.svg` |
| ![sim_card_chip](duifx/gui/resources/icon/sim_card_chip.svg) | sim_card_chip | `:icons/icon/sim_card_chip.svg` |
| ![Slave](duifx/gui/resources/icon/Slave.svg) | Slave | `:icons/icon/Slave.svg` |
| ![slr_back_side](duifx/gui/resources/icon/slr_back_side.svg) | slr_back_side | `:icons/icon/slr_back_side.svg` |
| ![smartphone_tablet](duifx/gui/resources/icon/smartphone_tablet.svg) | smartphone_tablet | `:icons/icon/smartphone_tablet.svg` |
| ![sms](duifx/gui/resources/icon/sms.svg) | sms | `:icons/icon/sms.svg` |
| ![sound_recording_copyright](duifx/gui/resources/icon/sound_recording_copyright.svg) | sound_recording_copyright | `:icons/icon/sound_recording_copyright.svg` |
| ![speaker](duifx/gui/resources/icon/speaker.svg) | speaker | `:icons/icon/speaker.svg` |
| ![sports_mode](duifx/gui/resources/icon/sports_mode.svg) | sports_mode | `:icons/icon/sports_mode.svg` |
| ![stack_of_photos](duifx/gui/resources/icon/stack_of_photos.svg) | stack_of_photos | `:icons/icon/stack_of_photos.svg` |
| ![start](duifx/gui/resources/icon/start.svg) | start | `:icons/icon/start.svg` |
| ![statistics](duifx/gui/resources/icon/statistics.svg) | statistics | `:icons/icon/statistics.svg` |
| ![support](duifx/gui/resources/icon/support.svg) | support | `:icons/icon/support.svg` |
| ![survey](duifx/gui/resources/icon/survey.svg) | survey | `:icons/icon/survey.svg` |
| ![switch_camera](duifx/gui/resources/icon/switch_camera.svg) | switch_camera | `:icons/icon/switch_camera.svg` |
| ![synchronize](duifx/gui/resources/icon/synchronize.svg) | synchronize | `:icons/icon/synchronize.svg` |
| ![tablet_android](duifx/gui/resources/icon/tablet_android.svg) | tablet_android | `:icons/icon/tablet_android.svg` |
| ![template](duifx/gui/resources/icon/template.svg) | template | `:icons/icon/template.svg` |
| ![timeline](duifx/gui/resources/icon/timeline.svg) | timeline | `:icons/icon/timeline.svg` |
| ![todo_list](duifx/gui/resources/icon/todo_list.svg) | todo_list | `:icons/icon/todo_list.svg` |
| ![touchscreen_smartphone](duifx/gui/resources/icon/touchscreen_smartphone.svg) | touchscreen_smartphone | `:icons/icon/touchscreen_smartphone.svg` |
| ![trademark](duifx/gui/resources/icon/trademark.svg) | trademark | `:icons/icon/trademark.svg` |
| ![tree_structure](duifx/gui/resources/icon/tree_structure.svg) | tree_structure | `:icons/icon/tree_structure.svg` |
| ![two_smartphones](duifx/gui/resources/icon/two_smartphones.svg) | two_smartphones | `:icons/icon/two_smartphones.svg` |
| ![Uncheck 1](duifx/gui/resources/icon/Uncheck%201.svg) | Uncheck 1 | `:icons/icon/Uncheck 1.svg` |
| ![Uncheck](duifx/gui/resources/icon/Uncheck.svg) | Uncheck | `:icons/icon/Uncheck.svg` |
| ![undo](duifx/gui/resources/icon/undo.svg) | undo | `:icons/icon/undo.svg` |
| ![unlock](duifx/gui/resources/icon/unlock.svg) | unlock | `:icons/icon/unlock.svg` |
| ![up](duifx/gui/resources/icon/up.svg) | up | `:icons/icon/up.svg` |
| ![up_left](duifx/gui/resources/icon/up_left.svg) | up_left | `:icons/icon/up_left.svg` |
| ![up_right](duifx/gui/resources/icon/up_right.svg) | up_right | `:icons/icon/up_right.svg` |
| ![upload](duifx/gui/resources/icon/upload.svg) | upload | `:icons/icon/upload.svg` |
| ![video_call](duifx/gui/resources/icon/video_call.svg) | video_call | `:icons/icon/video_call.svg` |
| ![video_file](duifx/gui/resources/icon/video_file.svg) | video_file | `:icons/icon/video_file.svg` |
| ![video_projector](duifx/gui/resources/icon/video_projector.svg) | video_projector | `:icons/icon/video_projector.svg` |
| ![view_details](duifx/gui/resources/icon/view_details.svg) | view_details | `:icons/icon/view_details.svg` |
| ![vip](duifx/gui/resources/icon/vip.svg) | vip | `:icons/icon/vip.svg` |
| ![voice_presentation](duifx/gui/resources/icon/voice_presentation.svg) | voice_presentation | `:icons/icon/voice_presentation.svg` |
| ![voicemail](duifx/gui/resources/icon/voicemail.svg) | voicemail | `:icons/icon/voicemail.svg` |
| ![webcam](duifx/gui/resources/icon/webcam.svg) | webcam | `:icons/icon/webcam.svg` |
| ![workflow](duifx/gui/resources/icon/workflow.svg) | workflow | `:icons/icon/workflow.svg` |

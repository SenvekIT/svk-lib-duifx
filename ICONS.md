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

| Icon | Name | Resource Path |
|------|------|---------------|
| ![3d Pointer](duifx/gui/resources/icon/3d%20Pointer.svg) | 3d Pointer | `:icons/icon/3d Pointer.svg` |
| ![About](duifx/gui/resources/icon/About.svg) | About | `:icons/icon/About.svg` |
| ![Add Bookmark](duifx/gui/resources/icon/Add%20Bookmark.svg) | Add Bookmark | `:icons/icon/Add Bookmark.svg` |
| ![Alert](duifx/gui/resources/icon/Alert.svg) | Alert | `:icons/icon/Alert.svg` |
| ![arrow-down-icon](duifx/gui/resources/icon/arrow-down-icon.svg) | arrow-down-icon | `:icons/icon/arrow-down-icon.svg` |
| ![arrow-up-icon](duifx/gui/resources/icon/arrow-up-icon.svg) | arrow-up-icon | `:icons/icon/arrow-up-icon.svg` |
| ![Bebo](duifx/gui/resources/icon/Bebo.svg) | Bebo | `:icons/icon/Bebo.svg` |
| ![Binoculars](duifx/gui/resources/icon/Binoculars.svg) | Binoculars | `:icons/icon/Binoculars.svg` |
| ![Bookmark](duifx/gui/resources/icon/Bookmark.svg) | Bookmark | `:icons/icon/Bookmark.svg` |
| ![Box](duifx/gui/resources/icon/Box.svg) | Box | `:icons/icon/Box.svg` |
| ![Briefcase](duifx/gui/resources/icon/Briefcase.svg) | Briefcase | `:icons/icon/Briefcase.svg` |
| ![Cancel](duifx/gui/resources/icon/Cancel.svg) | Cancel | `:icons/icon/Cancel.svg` |
| ![Check All](duifx/gui/resources/icon/Check%20All.svg) | Check All | `:icons/icon/Check All.svg` |
| ![Check Mark](duifx/gui/resources/icon/Check%20Mark.svg) | Check Mark | `:icons/icon/Check Mark.svg` |
| ![Checkmark](duifx/gui/resources/icon/Checkmark.svg) | Checkmark | `:icons/icon/Checkmark.svg` |
| ![Clock](duifx/gui/resources/icon/Clock.svg) | Clock | `:icons/icon/Clock.svg` |
| ![Close](duifx/gui/resources/icon/Close.svg) | Close | `:icons/icon/Close.svg` |
| ![Connect](duifx/gui/resources/icon/Connect.svg) | Connect | `:icons/icon/Connect.svg` |
| ![Contacts](duifx/gui/resources/icon/Contacts.svg) | Contacts | `:icons/icon/Contacts.svg` |
| ![Cursor](duifx/gui/resources/icon/Cursor.svg) | Cursor | `:icons/icon/Cursor.svg` |
| ![Delete](duifx/gui/resources/icon/Delete.svg) | Delete | `:icons/icon/Delete.svg` |
| ![Delete Bookmark](duifx/gui/resources/icon/Delete%20Bookmark.svg) | Delete Bookmark | `:icons/icon/Delete Bookmark.svg` |
| ![Document](duifx/gui/resources/icon/Document.svg) | Document | `:icons/icon/Document.svg` |
| ![Done](duifx/gui/resources/icon/Done.svg) | Done | `:icons/icon/Done.svg` |
| ![Download](duifx/gui/resources/icon/Download.svg) | Download | `:icons/icon/Download.svg` |
| ![Download from the Cloud](duifx/gui/resources/icon/Download%20from%20the%20Cloud.svg) | Download from the Cloud | `:icons/icon/Download from the Cloud.svg` |
| ![Dribbble](duifx/gui/resources/icon/Dribbble.svg) | Dribbble | `:icons/icon/Dribbble.svg` |
| ![Dx](duifx/gui/resources/icon/Dx.svg) | Dx | `:icons/icon/Dx.svg` |
| ![Edit](duifx/gui/resources/icon/Edit.svg) | Edit | `:icons/icon/Edit.svg` |
| ![External Link](duifx/gui/resources/icon/External%20Link.svg) | External Link | `:icons/icon/External Link.svg` |
| ![Facebook](duifx/gui/resources/icon/Facebook.svg) | Facebook | `:icons/icon/Facebook.svg` |
| ![Favorites](duifx/gui/resources/icon/Favorites.svg) | Favorites | `:icons/icon/Favorites.svg` |
| ![Female User](duifx/gui/resources/icon/Female%20User.svg) | Female User | `:icons/icon/Female User.svg` |
| ![File](duifx/gui/resources/icon/File.svg) | File | `:icons/icon/File.svg` |
| ![File Error](duifx/gui/resources/icon/File%20Error.svg) | File Error | `:icons/icon/File Error.svg` |
| ![Folder](duifx/gui/resources/icon/Folder.svg) | Folder | `:icons/icon/Folder.svg` |
| ![For You](duifx/gui/resources/icon/For%20You.svg) | For You | `:icons/icon/For You.svg` |
| ![Google Plus](duifx/gui/resources/icon/Google%20Plus.svg) | Google Plus | `:icons/icon/Google Plus.svg` |
| ![Hand Cursor](duifx/gui/resources/icon/Hand%20Cursor.svg) | Hand Cursor | `:icons/icon/Hand Cursor.svg` |
| ![Home](duifx/gui/resources/icon/Home.svg) | Home | `:icons/icon/Home.svg` |
| ![Icons8](duifx/gui/resources/icon/Icons8.svg) | Icons8 | `:icons/icon/Icons8.svg` |
| ![Idea](duifx/gui/resources/icon/Idea.svg) | Idea | `:icons/icon/Idea.svg` |
| ![Image File](duifx/gui/resources/icon/Image%20File.svg) | Image File | `:icons/icon/Image File.svg` |
| ![Info](duifx/gui/resources/icon/Info.svg) | Info | `:icons/icon/Info.svg` |
| ![Instagram](duifx/gui/resources/icon/Instagram.svg) | Instagram | `:icons/icon/Instagram.svg` |
| ![Key](duifx/gui/resources/icon/Key.svg) | Key | `:icons/icon/Key.svg` |
| ![LinkedIn](duifx/gui/resources/icon/LinkedIn.svg) | LinkedIn | `:icons/icon/LinkedIn.svg` |
| ![LinkedIn Circled](duifx/gui/resources/icon/LinkedIn%20Circled.svg) | LinkedIn Circled | `:icons/icon/LinkedIn Circled.svg` |
| ![Lock](duifx/gui/resources/icon/Lock.svg) | Lock | `:icons/icon/Lock.svg` |
| ![Lunacy](duifx/gui/resources/icon/Lunacy.svg) | Lunacy | `:icons/icon/Lunacy.svg` |
| ![Mailbox](duifx/gui/resources/icon/Mailbox.svg) | Mailbox | `:icons/icon/Mailbox.svg` |
| ![Menu](duifx/gui/resources/icon/Menu.svg) | Menu | `:icons/icon/Menu.svg` |
| ![Music](duifx/gui/resources/icon/Music.svg) | Music | `:icons/icon/Music.svg` |
| ![MySpace](duifx/gui/resources/icon/MySpace.svg) | MySpace | `:icons/icon/MySpace.svg` |
| ![Myspace 3](duifx/gui/resources/icon/Myspace%203.svg) | Myspace 3 | `:icons/icon/Myspace 3.svg` |
| ![Myspace App](duifx/gui/resources/icon/Myspace%20App.svg) | Myspace App | `:icons/icon/Myspace App.svg` |
| ![News](duifx/gui/resources/icon/News.svg) | News | `:icons/icon/News.svg` |
| ![No Synchronize](duifx/gui/resources/icon/No%20Synchronize.svg) | No Synchronize | `:icons/icon/No Synchronize.svg` |
| ![Nodes](duifx/gui/resources/icon/Nodes.svg) | Nodes | `:icons/icon/Nodes.svg` |
| ![object_directory](duifx/gui/resources/icon/object_directory.svg) | object_directory | `:icons/icon/object_directory.svg` |
| ![Opened Folder](duifx/gui/resources/icon/Opened%20Folder.svg) | Opened Folder | `:icons/icon/Opened Folder.svg` |
| ![Padlock](duifx/gui/resources/icon/Padlock.svg) | Padlock | `:icons/icon/Padlock.svg` |
| ![Picture](duifx/gui/resources/icon/Picture.svg) | Picture | `:icons/icon/Picture.svg` |
| ![Pinterest](duifx/gui/resources/icon/Pinterest.svg) | Pinterest | `:icons/icon/Pinterest.svg` |
| ![Plus](duifx/gui/resources/icon/Plus.svg) | Plus | `:icons/icon/Plus.svg` |
| ![Puzzle](duifx/gui/resources/icon/Puzzle.svg) | Puzzle | `:icons/icon/Puzzle.svg` |
| ![qr](duifx/gui/resources/icon/qr.svg) | qr | `:icons/icon/qr.svg` |
| ![record](duifx/gui/resources/icon/record.svg) | record | `:icons/icon/record.svg` |
| ![Reddit](duifx/gui/resources/icon/Reddit.svg) | Reddit | `:icons/icon/Reddit.svg` |
| ![Refresh](duifx/gui/resources/icon/Refresh.svg) | Refresh | `:icons/icon/Refresh.svg` |
| ![Remove Bookmark](duifx/gui/resources/icon/Remove%20Bookmark.svg) | Remove Bookmark | `:icons/icon/Remove Bookmark.svg` |
| ![Restart](duifx/gui/resources/icon/Restart.svg) | Restart | `:icons/icon/Restart.svg` |
| ![Risk](duifx/gui/resources/icon/Risk.svg) | Risk | `:icons/icon/Risk.svg` |
| ![Round](duifx/gui/resources/icon/Round.svg) | Round | `:icons/icon/Round.svg` |
| ![Scroll](duifx/gui/resources/icon/Scroll.svg) | Scroll | `:icons/icon/Scroll.svg` |
| ![Search](duifx/gui/resources/icon/Search.svg) | Search | `:icons/icon/Search.svg` |
| ![Services](duifx/gui/resources/icon/Services.svg) | Services | `:icons/icon/Services.svg` |
| ![Settings](duifx/gui/resources/icon/Settings.svg) | Settings | `:icons/icon/Settings.svg` |
| ![Share](duifx/gui/resources/icon/Share.svg) | Share | `:icons/icon/Share.svg` |
| ![Share_1](duifx/gui/resources/icon/Share_1.svg) | Share_1 | `:icons/icon/Share_1.svg` |
| ![Share_2](duifx/gui/resources/icon/Share_2.svg) | Share_2 | `:icons/icon/Share_2.svg` |
| ![Speech Bubble](duifx/gui/resources/icon/Speech%20Bubble.svg) | Speech Bubble | `:icons/icon/Speech Bubble.svg` |
| ![StumbleUpon](duifx/gui/resources/icon/StumbleUpon.svg) | StumbleUpon | `:icons/icon/StumbleUpon.svg` |
| ![Sun](duifx/gui/resources/icon/Sun.svg) | Sun | `:icons/icon/Sun.svg` |
| ![Synchronize](duifx/gui/resources/icon/Synchronize.svg) | Synchronize | `:icons/icon/Synchronize.svg` |
| ![Tick Box](duifx/gui/resources/icon/Tick%20Box.svg) | Tick Box | `:icons/icon/Tick Box.svg` |
| ![Toolbox](duifx/gui/resources/icon/Toolbox.svg) | Toolbox | `:icons/icon/Toolbox.svg` |
| ![Trash](duifx/gui/resources/icon/Trash.svg) | Trash | `:icons/icon/Trash.svg` |
| ![Trash Can](duifx/gui/resources/icon/Trash%20Can.svg) | Trash Can | `:icons/icon/Trash Can.svg` |
| ![Tumblr](duifx/gui/resources/icon/Tumblr.svg) | Tumblr | `:icons/icon/Tumblr.svg` |
| ![Unavailable](duifx/gui/resources/icon/Unavailable.svg) | Unavailable | `:icons/icon/Unavailable.svg` |
| ![Uncheck](duifx/gui/resources/icon/Uncheck.svg) | Uncheck | `:icons/icon/Uncheck.svg` |
| ![Uncheck 1](duifx/gui/resources/icon/Uncheck%201.svg) | Uncheck 1 | `:icons/icon/Uncheck 1.svg` |
| ![Uncheck All](duifx/gui/resources/icon/Uncheck%20All.svg) | Uncheck All | `:icons/icon/Uncheck All.svg` |
| ![Undelete](duifx/gui/resources/icon/Undelete.svg) | Undelete | `:icons/icon/Undelete.svg` |
| ![Upload to the Cloud](duifx/gui/resources/icon/Upload%20to%20the%20Cloud.svg) | Upload to the Cloud | `:icons/icon/Upload to the Cloud.svg` |
| ![User Male](duifx/gui/resources/icon/User%20Male.svg) | User Male | `:icons/icon/User Male.svg` |
| ![Wrench](duifx/gui/resources/icon/Wrench.svg) | Wrench | `:icons/icon/Wrench.svg` |
| ![X](duifx/gui/resources/icon/X.svg) | X | `:icons/icon/X.svg` |
| ![X2](duifx/gui/resources/icon/X2.svg) | X2 | `:icons/icon/X2.svg` |

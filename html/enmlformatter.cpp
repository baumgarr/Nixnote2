/*********************************************************************************
NixNote - An open-source client for the Evernote service.
Copyright (C) 2013 Randy Baumgarte

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
***********************************************************************************/


#include "enmlformatter.h"
#include "sql/resourcetable.h"
#include "global.h"
#include "utilities/encrypt.h"

#include <QFileIconProvider>
#include <QIcon>
#include <QMessageBox>

#include <iostream>
using namespace std;

extern Global global;

/* Constructor. */
EnmlFormatter::EnmlFormatter(QObject *parent) :
    QObject(parent)
{
    //doc = new QDomDocument();
    formattingError = false;
}

/* Return the formatted content */
QString EnmlFormatter::getEnml() {
    return this->content;
}


void EnmlFormatter::setHtml(QString h) {
    content.clear();
    content.append(h);
}

/* Take the ENML note and transform it into HTML that WebKit will
  not complain about */
QByteArray EnmlFormatter::rebuildNoteEnml() {
    resources.clear();
    QByteArray b;
    qint32 index;

    content.replace("</input>","");

    // Strip off HTML header
    index = content.indexOf("<body");
    index = content.indexOf(">", index)+1;
    content.remove(0,index);
    index = content.indexOf("</body");
    content.truncate(index);
    b.append("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
    b.append("<!DOCTYPE en-note SYSTEM 'http://xml.evernote.com/pub/enml2.dtd'>\n");
    b.append("<html><head><title></title></head>");
    b.append("<body style=\"word-wrap: break-word; -webkit-nbsp-mode: space; -webkit-line-break: after-white-space;\" >");
    b.append(content);
    b.append("</body></html>");
    content.clear();
    content = b;

    // Run it through "tidy".  It is a program which will fix any invalid HTML
    // and give us the results back through stdout.  In a perfect world this
    // wouldn't be needed, but WebKit doesn't always give back good HTML.
    QProcess tidyProcess;
    tidyProcess.start("tidy -raw -asxhtml -q -m -u -utf8 ", QIODevice::ReadWrite|QIODevice::Unbuffered);
    QLOG_DEBUG() << "Starting tidy " << tidyProcess.waitForStarted();
    tidyProcess.waitForStarted();
    tidyProcess.write(content);
    tidyProcess.closeWriteChannel();
    tidyProcess.waitForFinished();
    QLOG_DEBUG() << "Stopping tidy " << tidyProcess.waitForFinished() << " Return Code: " << tidyProcess.state();
    QString errors(tidyProcess.readAllStandardError());
    QStringList errorList = errors.split("\n");
    for (int e=0; e<errorList.size(); e++) {
        if (errorList[e].indexOf("<img> proprietary attribute \"type\"") < 0 &&
                errorList[e].indexOf("<img> proprietary attribute \"oncontextmenu\"") < 0 &&
                errorList[e].indexOf("<img> proprietary attribute \"hash\"") < 0 &&
                errorList[e].indexOf("<img> proprietary attribute \"en-tag\"") < 0 &&
                errorList[e].indexOf("<img> proprietary attribute \"lid\"") < 0 &&
                errorList[e].indexOf("<img> proprietary attribute \"oncontextmenu\"") < 0 &&
                errorList[e].indexOf("<img> lacks \"alt\" attribute") < 0
            ) {
            QLOG_DEBUG() << errorList[e];
        }
    }

    content.clear();
    content.append(tidyProcess.readAllStandardOutput());
    tidyProcess.close();
    if (content == "") {
        formattingError = true;
        return "";
    }

    // Tidy puts this in place, but we don't really need it.
    content.replace("<form>", "");
    content.replace("</form>", "");

    index = content.indexOf("<body");
    content.remove(0,index);
    content.prepend("<style>img { height:auto; width:auto; max-height:auto; max-width:100%; }</style>");
    content.prepend("<head><meta http-equiv=\"content-type\" content=\"text-html; charset=utf-8\"></head>");
    content.prepend("<html>");
    content.append("</html>");
    content = fixEncryptionTags(content);

    QWebPage page;
    QEventLoop loop;
    page.mainFrame()->setContent(content);
    QObject::connect(&page, SIGNAL(loadFinished(bool)), &loop, SLOT(quit()));
    loop.exit();

    QWebElement element = page.mainFrame()->documentElement();
    QStringList tags = findAllTags(element);

    for (int i=0; i<tags.size(); i++) {
        QString tag = tags[i];
        QWebElementCollection anchors = page.mainFrame()->findAllElements(tag);
        foreach (QWebElement element, anchors) {
            //QLOG_DEBUG() << "Processing tag name: " << element.tagName();
            if (element.tagName().toLower() == "input") {
                processTodo(element);
            } else if (element.tagName().toLower() == "a") {
                fixLinkNode(element);
            } else if (element.tagName().toLower() == "object") {
                fixObjectNode(element);
            } else if (element.tagName().toLower() == "img") {
                fixImgNode(element);
            } else if (element.tagName().toLower() == "span"){
                fixSpanNode(element);
            } else if (!isElementValid(element.tagName()))
                element.removeFromDocument();
        }
    }
    content.clear();
    content.append(element.toOuterXml());

    // Strip off HTML header
    index = content.indexOf("<body");
    index = content.indexOf(">", index)+1;
    content.remove(0,index);
    index = content.indexOf("</body");
    content.truncate(index);
    b.clear();
    b.append("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
    b.append("<!DOCTYPE en-note SYSTEM 'http://xml.evernote.com/pub/enml2.dtd'>");
    b.append("<en-note style=\"word-wrap: break-word; -webkit-nbsp-mode: space; -webkit-line-break: after-white-space;\" >");
    b.append(content);
    b.append("</en-note>");
    content.clear();
    content = b;

    postXmlFix();
    return content;
}




QStringList EnmlFormatter::findAllTags(QWebElement &element) {
    QStringList tags;
    QString body = element.toOuterXml();
    body = body.replace("</","<").replace("/>"," ").replace(">", " ");
    int i = body.indexOf("<body")+1
            ;
    // Look throug and get a list of all possible tags
    i = body.indexOf("<", i);

    while (i>0) {
        int eot = body.indexOf(" ",i);
        QString tag = body.mid(i+1, eot-i-1);
        if (!tags.contains(tag) && tag != "body")
            tags.append(tag);
        i=body.indexOf("<", eot);
        if (tag == "body")
            i=-1;
    }
    return tags;
}


void EnmlFormatter::processTodo(QWebElement &node) {
    bool checked=false;
    if (node.hasAttribute("checked"))
        checked = true;
    node.removeAttribute("style");
    node.removeAttribute("type");
    removeInvalidAttributes(node);
    if (checked)
        node.setAttribute("checked", "true");
    node.setOuterXml(node.toOuterXml().replace("<input", "<en-todo").replace("</input", "</en-todo"));
}



void EnmlFormatter::fixSpanNode(QWebElement &e) {
    e.removeAttribute("id");
}




void EnmlFormatter::fixObjectNode(QWebElement &e) {
    QString type = e.attribute("type", "");
    if (type == "application/pdf") {
        qint32 lid = e.attribute("lid", "0").toInt();
        e.removeAttribute("width");
        e.removeAttribute("height");
        e.removeAttribute("lid");
        e.removeAttribute("border");
        if (lid>0) {
            resources.append(lid);
             e.setOuterXml(e.toOuterXml().replace("<object", "<en-media").replace("</object", "</en-media"));
        }
        removeInvalidAttributes(e);
    } else {
        e.removeFromDocument();
    }
}



void EnmlFormatter::fixEnCryptNode(QWebElement &e) {
    QString crypt = e.attribute("value");
    e.removeAttribute("value");
    QDomText cryptValue = doc.createTextNode(crypt);
    //e.appendChild(cryptValue);
}


void EnmlFormatter::fixImgNode(QWebElement &e) {
    QString enType = e.attribute("en-tag", "");

    // Check if we have an en-crypt tag.  Change it from an img to en-crypt
    if (enType.toLower() == "en-crypt") {
        QString encrypted = e.attribute("alt");
        QString cipher = e.attribute("cipher", "RC2");
        QString hint = e.attribute("hint", "");
        QString length = e.attribute("length", "64");
        e.removeAttribute("onmouseover");
        e.removeAttribute("name");
        e.removeAttribute("alt");
        e.removeAttribute("en-tag");
        e.removeAttribute("contenteditable");
        e.removeAttribute("style");
        removeInvalidAttributes(e);
        e.setInnerXml(encrypted);
        e.setOuterXml("<en-crypt cipher=\"" +cipher +"\" length=\"" +
                      length + "\" hint=\"" +hint
                      +"\">"+encrypted+"</en-crypt>");
//        e.setOuterXml(e.toOuterXml().replace("<img", "<en-crypt").replace("</img", "</en-crypt"));
        return;
    }

    // Check if we have a temporary image.  If so, remove it
    if (enType.toLower() ==  "temporary") {;
        e.removeFromDocument();
        return;
    }


    // Latex images are really just img tags, so we now handle them later
    // Check if we have a LaTeX image.  Remove the parent link tag
//    if (enType.toLower() ==  "en-latex") {
//        enType = "en-media";
//        parent.parentNode().replaceChild(e, parent);
//    }


    // If we've gotten this far, we have an en-media tag
    e.removeAttribute("en-tag");
    int lid = e.attribute("lid").toInt();
    resources.append(lid);
    removeInvalidAttributes(e);
    e.setOuterXml(e.toOuterXml().replace("<img", "<en-media").replace("</img", "</en-media"));
}



void EnmlFormatter::fixLinkNode(QWebElement e) {
    QString enTag = e.attribute("en-tag", "");
    if (enTag.toLower() == "en-media") {
        resources.append(e.attribute("lid").toInt());
        e.removeAttribute("style");
        e.removeAttribute("href");
        e.removeAttribute("title");
        removeInvalidAttributes(e);
        e.removeAllChildren();
        QString newXml = e.toOuterXml();
        newXml.replace("<a", "<en-media");
        newXml.replace("</a>", "</en-media>");
        e.setOuterXml(newXml);
    }
    QString latex = e.attribute("href", "");
    if (latex.toLower().startsWith("latex://")) {
        removeInvalidAttributes(e);
        e.removeAttribute("title");
        e.removeAttribute("href");
        e.setOuterXml(e.toInnerXml());
    }
    removeInvalidAttributes(e);
}






bool EnmlFormatter::isAttributeValid(QString attribute) {
    if (attribute.startsWith("on")) return false;
    if (attribute == "id") return false;
    if (attribute == "class") return false;
    if (attribute == "accesskey") return false;
    if (attribute == "data") return false;
    if (attribute == "dynsrc") return false;
    if (attribute == "tabindex") return false;

    // These are things that are NixNote specific
    if (attribute == "en-tag") return false;
    if (attribute == "src") return false;
    if (attribute == "en-new") return false;
    if (attribute == "guid") return false;
    if (attribute == "lid") return false;

    return true;
}


bool EnmlFormatter::isElementValid(QString element) {
    element = element.trimmed().toLower();
    //QLOG_DEBUG() << "Checking tag " << element;
    if (element == "a") return true;
    if (element == "abbr") return true;
    if (element == "acronym") return true;
    if (element == "address") return true;
    if (element == "area") return true;
    if (element == "b") return true;
    if (element == "bdo") return true;
    if (element == "big") return true;
    if (element == "blockquote") return true;
    if (element == "br") return true;
    if (element == "caption") return true;
    if (element == "center") return true;
    if (element == "cite") return true;
    if (element == "code") return true;
    if (element == "col") return true;
    if (element == "colgroup") return true;
    if (element == "dd") return true;
    if (element == "del") return true;
    if (element == "dfn") return true;
    if (element == "div") return true;
    if (element == "dl") return true;
    if (element == "dt") return true;
    if (element == "em") return true;
    if (element == "en-media") return true;
    if (element == "en-crypt") return true;
    if (element == "en-todo") return true;
    if (element == "en-note") return true;
    if (element == "font") return true;
    if (element == "h1") return true;
    if (element == "h1") return true;
    if (element == "h1") return true;
    if (element == "h2") return true;
    if (element == "h3") return true;
    if (element == "h4") return true;
    if (element == "h5") return true;
    if (element == "h6") return true;
    if (element == "hr") return true;
    if (element == "i") return true;
    if (element == "img") return true;
    if (element == "ins") return true;
    if (element == "kbd") return true;
    if (element == "li") return true;
    if (element == "map") return true;
    if (element == "ol") return true;
    if (element == "p") return true;
    if (element == "pre") return true;
    if (element == "q") return true;
    if (element == "s") return true;
    if (element == "samp") return true;
    if (element == "small") return true;
    if (element == "span") return true;
    if (element == "strike") return true;
    if (element == "strong") return true;
    if (element == "sub") return true;
    if (element == "sup") return true;
    if (element == "table") return true;
    if (element == "tbody") return true;
    if (element == "td") return true;
    if (element == "tfoot") return true;
    if (element == "th") return true;
    if (element == "thread") return true;
    if (element == "title") return true;
    if (element == "tr") return true;
    if (element == "tt") return true;
    if (element == "u") return true;
    if (element == "ul") return true;
    if (element == "var") return true;
    if (element == "xmp") return true;

    QLOG_DEBUG() << "WARNING: " << element << " is invalid";
    return false;

}



void EnmlFormatter::removeInvalidAttributes(QWebElement &node) {
    // Remove any invalid attributes
    QStringList attributes = node.attributeNames();
    for (int i=0; i<attributes.size(); i++) {
        if (!isAttributeValid(attributes[i])) {
            node.removeAttribute(attributes[i]);
        }
    }
}





void EnmlFormatter::postXmlFix() {

    int pos;
    content = content.replace("<hr>", "<hr/>");

    // Fix the <br> tags
    content = content.replace("<br clear=\"none\">", "<br/>");
    pos = content.indexOf("<br");
    if (pos > 0) {
        QLOG_DEBUG() << "<br found.  Beginning fix";
    }
    while (pos > 0) {
        int endPos = content.indexOf(">", pos);
        int tagEndPos = content.indexOf("/>", pos);

        // Check the next /> end tag.  If it is beyond the end
        // of the current tag or if it doesn't exist then we
        // need to fix the end of the img
        if (tagEndPos <= 0 || tagEndPos > endPos) {
            content = content.mid(0, endPos) + QByteArray("/>") +content.mid(endPos+1);
        }
         pos = content.indexOf("<br", pos+1);
    }


    // Fix the <en-todo> tags
    content = content.replace("</en-todo>", "");
    pos = content.indexOf("<en-todo");
    if (pos > 0) {
        QLOG_DEBUG() << "<en-todo found.  Beginning fix";
    }
    while (pos > 0) {
        int endPos = content.indexOf(">", pos);
        int tagEndPos = content.indexOf("/>", pos);

        // Check the next /> end tag.  If it is beyond the end
        // of the current tag or if it doesn't exist then we
        // need to fix the end of the img
        if (tagEndPos <= 0 || tagEndPos > endPos) {
            content = content.mid(0, endPos) + QByteArray("/>") +content.mid(endPos+1);
        }
         pos = content.indexOf("<en-todo", pos+1);
    }

    // Fix any <img> tags
    content = content.replace("</en-media>", "");
    pos = content.indexOf("<en-media");
    if (pos > 0) {
        QLOG_DEBUG() << "<en-media found.  Beginning fix";
    }
    while (pos > 0) {
        int endPos = content.indexOf(">", pos);
        int tagEndPos = content.indexOf("/>", pos);

        // Check the next /> end tag.  If it is beyond the end
        // of the current tag or if it doesn't exist then we
        // need to fix the end of the img
        if (tagEndPos <= 0 || tagEndPos > endPos) {
            content = content.mid(0, endPos) + QByteArray("/>") +content.mid(endPos+1);
        }
        pos = content.indexOf("<en-media", pos+1);
    }
}



QByteArray EnmlFormatter::fixEncryptionTags(QByteArray newContent) {
    int endPos, startPos, endData,slotStart, slotEnd;
    QByteArray eTag = "<table class=\"en-crypt-temp\"";
    for (int i=newContent.indexOf(eTag); i>0; i = newContent.indexOf(eTag,i+1)) {
        slotStart = newContent.indexOf("slot", i+1)+6;
        slotEnd = newContent.indexOf("\"",slotStart+1);
        QString slot = newContent.mid(slotStart, slotEnd-slotStart);
        slot = slot.replace("\"", "");
        startPos = newContent.indexOf("<td>", i+1)+4;
        endData = newContent.indexOf("</td>",startPos);
        QString text = newContent.mid(startPos,endData-startPos);
        endPos = newContent.indexOf("</table>",i+1)+8;

        // Encrypt the text
        QPair<QString, QString> pair = global.passwordSafe.value(slot);
        QString password = pair.first;
        QString hint = pair.second;
        EnCrypt crypt;
        QString encrypted;
        crypt.encrypt(encrypted, text, password);

        // replace the table with an en-crypt tag.
        QByteArray start = newContent.mid(0,i-1);
        QByteArray end = newContent.mid(endPos);
        newContent.clear();
        newContent.append(start);
        newContent.append(QByteArray("<en-crypt cipher=\"RC2\" length=\"64\" hint=\""));
        newContent.append(hint.toLocal8Bit());
        newContent.append(QByteArray("\">"));
        newContent.append(encrypted.toLocal8Bit());
        newContent.append(QByteArray("</en-crypt>"));
        newContent.append(end);
    }
    return newContent;
}

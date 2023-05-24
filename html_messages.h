#ifndef HTML_MESSAGES_H
#define HTML_MESSAGES_H

#include <string>

class Messages
{
public:
    static constexpr auto& help_message(){return "<b><u>Тут набор команд</u></b> \
                                                 <b>/price</b> - текущий курс \
                                                 <b>/line</b> - запрос линии \
                                                 <b>/line aaaa.bbb</b> - установка линии \
                                                 <b>/stat</b> - данные свечи/линии \
                                                 <b>/account</b> - данные аккаунта";}

    static constexpr auto& start_message(){return "<b>Привет!</b> \
                                                   <pre>Появились деньги?</pre> \
                                                   <pre>Нет проблем! Сейчас я все исправлю!</pre>";}

    static constexpr auto& stop_message(){return "<b>Пока!</b> \
                                                  <pre>Возвращайся дорогой!</pre>";}



static constexpr auto& sample(){return "<b>bold</b>, <strong>bold</strong> \
                                          <i>italic</i>, <em>italic</em> \
                                          <a href=\"URL\">inline URL</a> \
                                          <code>inline fixed-width code</code> \
                                          <pre>pre-formatted fixed-width code block</pre>";}

};

#endif // HTML_MESSAGES_H

/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   constexpr.hpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: bstorck <marvin@42.fr>                     +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/07/22 16:17:06 by bstorck           #+#    #+#             */
/*   Updated: 2026/07/22 21:03:47 by bstorck          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CONSTEXPR_HPP
#define CONSTEXPR_HPP

namespace http {

	static const char _ = ' ';
	static const char CR = '\r';
	static const char LF = '\n';
	static const char LFLF[] = "\n\n";
	static const char CRLF[] = "\r\n";
	static const char CRLFCRLF[] = "\r\n\r\n";
	static const char V_1_0[] = "HTTP/1.0";
	static const char V_1_1[] = "HTTP/1.1";

}

namespace tag {

	static const char DOC[] = "<!DOCTYPE html>";
	static const char HTML[] = "<html lang=\"en\">";
	static const char HEAD[] = "<head>";
	static const char TITLE[] = "<title>";
	static const char BODY[] = "<body>";
	static const char H1[] = "<h1>";
	static const char UL[] = "<ul>";
	static const char LI[] = "<li>";
	static const char A[] = "<a ";
	static const char HREF[] = "href=\"";
	static const char BR[] = "<br>";
	static const char TAB[] = "<span style=\"display:inline-block; width: 4em;\"></span>";
	static const char _HTML[] = "</html>";
	static const char _HEAD[] = "</head>";
	static const char _TITLE[] = "</title>";
	static const char _BODY[] = "</body>";
	static const char _H1[] = "</h1>";
	static const char _UL[] = "</ul>";
	static const char _LI[] = "</li>";
	static const char _A[] = "</a>";
	static const char _HREF[] = "\">";

}

namespace define {

	static const char META[] = "<meta charset=\"utf-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">";
	static const char FAVICON[] = "<link rel=\"icon\" type=\"image/x-icon\" href=\"/images/icons/favicon.ico\"><link rel=\"icon\" type=\"image/png\" sizes=\"32x32\" href=\"/images/icons/favicon-32x32.png\"><link rel=\"icon\" type=\"image/png\" sizes=\"16x16\" href=\"/images/icons/favicon-16x16.png\"><link rel=\"apple-touch-icon\" sizes=\"180x180\" href=\"/images/icons/apple-touch-icon.png\"><link rel=\"manifest\" href=\"/images/icons/site.webmanifest\">";
	static const char STYLE[] = "<link rel=\"stylesheet\" href=\"/css/style.css\">";

}

namespace button {

	static const char DELETE_[] = "<button class=\"delete\" data-url=\"";
	static const char _DELETE[] = "\">Delete</button>";

	static const char SCRIPT[] = "<script>document.querySelectorAll(\".delete\").forEach(function (button) {button.addEventListener(\"click\", function () {fetch(button.dataset.url, {method: \"DELETE\"}).then(function (response) {if (response.status === 204) {location.reload();}});});});</script>";

	static const char LEGACY[] = "<script>var buttons = document.getElementsByClassName(\"delete\");for (var i = 0; i < buttons.length; i++) {buttons[i].addEventListener(\"click\", function () {fetch(this.dataset.url, {method: \"DELETE\"}).then(function (response) {if (response.status === 204) {location.reload();}});});}</script>";

}

#endif

// link_to_clipboard.cpp
#include <windows.h>
#include <string>
#include <sstream>
#include <iomanip>
#include <iostream>

// Build a CF_HTML-compliant string from an HTML fragment,
// wrapped in <html><body>…</body></html>
std::string MakeCfHtml(const std::string& fragment) {
    // 1. Header template with 10-digit placeholders
    const std::string header =
        "Version:0.9\r\n"
        "StartHTML:##########\r\n"
        "EndHTML:##########\r\n"
        "StartFragment:##########\r\n"
        "EndFragment:##########\r\n";

    // 2. Markers and wrappers
    const std::string startFragMarker = "<!--StartFragment-->";
    const std::string endFragMarker = "<!--EndFragment-->";
    const std::string htmlOpen = "<html><body>";
    const std::string htmlClose = "</body></html>";

    // 3. Assemble full HTML (unpatched)
    std::string html = header
        + htmlOpen
        + startFragMarker
        + fragment
        + endFragMarker
        + htmlClose;

    // 4. Compute byte-offsets
    size_t ofsStartHTML = header.size();
    size_t ofsStartFragment = ofsStartHTML
        + htmlOpen.size()
        + startFragMarker.size();
    size_t ofsEndFragment = ofsStartFragment
        + fragment.size();
    size_t ofsEndHTML = header.size()
        + htmlOpen.size()
        + startFragMarker.size()
        + fragment.size()
        + endFragMarker.size()
        + htmlClose.size();

    auto pad10 = [](size_t n) {
        std::ostringstream os;
        os << std::setw(10) << std::setfill('0') << n;
        return os.str();
        };

    // 5. Patch in the offsets (in order!)
    html.replace(html.find("##########"), 10, pad10(ofsStartHTML));
    html.replace(html.find("##########"), 10, pad10(ofsEndHTML));
    html.replace(html.find("##########"), 10, pad10(ofsStartFragment));
    html.replace(html.find("##########"), 10, pad10(ofsEndFragment));

    return html;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: link_to_clipboard.exe <URL> <Link Text>\n";
        return 1;
    }

    std::string url = argv[1];
    std::string txt = argv[2];
    // Your HTML fragment
    std::string frag = "<a href=\"" + url + "\">" + txt + "</a>";
    // Build CF_HTML block
    std::string cfHtml = MakeCfHtml(frag);
    // Plain-text fallback
    std::string plain = txt + " (" + url + ")";

    if (!OpenClipboard(nullptr)) {
        std::cerr << "Error: could not open clipboard\n";
        return 1;
    }
    EmptyClipboard();

    // 1) CF_HTML
    UINT fmtHtml = RegisterClipboardFormatA("HTML Format");
    HGLOBAL hHtml = GlobalAlloc(GMEM_MOVEABLE, cfHtml.size() + 1);
    if (!hHtml) { CloseClipboard(); return 1; }
    {
        char* p = static_cast<char*>(GlobalLock(hHtml));
        memcpy(p, cfHtml.c_str(), cfHtml.size() + 1);
        GlobalUnlock(hHtml);
        SetClipboardData(fmtHtml, hHtml);
    }

    // 2) CF_UNICODETEXT fallback
    int wlen = MultiByteToWideChar(CP_UTF8, 0, plain.c_str(), -1, nullptr, 0);
    HGLOBAL hText = GlobalAlloc(GMEM_MOVEABLE, wlen * sizeof(wchar_t));
    if (!hText) { CloseClipboard(); return 1; }
    {
        wchar_t* pw = static_cast<wchar_t*>(GlobalLock(hText));
        MultiByteToWideChar(CP_UTF8, 0, plain.c_str(), -1, pw, wlen);
        GlobalUnlock(hText);
        SetClipboardData(CF_UNICODETEXT, hText);
    }

    CloseClipboard();
    return 0;
}

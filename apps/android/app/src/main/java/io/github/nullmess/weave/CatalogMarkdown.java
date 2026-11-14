package io.github.nullmess.weave;

import android.content.Context;
import android.widget.TextView;
import java.net.URI;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import io.noties.markwon.Markwon;
import io.noties.markwon.ext.strikethrough.StrikethroughPlugin;
import io.noties.markwon.ext.tables.TablePlugin;
import io.noties.markwon.ext.tasklist.TaskListPlugin;
import io.noties.markwon.html.HtmlPlugin;
import io.noties.markwon.image.ImagesPlugin;

/** Render official app descriptions as GitHub-flavored Markdown in the native UI. */
final class CatalogMarkdown {
    private final Markwon markwon;
    private static final Pattern LINK = Pattern.compile("(!?\\[[^]\\n]*\\])\\(([^)\\n]+)\\)");
    CatalogMarkdown(Context context) {
        markwon = Markwon.builder(context)
                .usePlugin(StrikethroughPlugin.create())
                .usePlugin(TablePlugin.create(context))
                .usePlugin(TaskListPlugin.create(context))
                .usePlugin(ImagesPlugin.create())
                .usePlugin(HtmlPlugin.create())
                .build();
    }
    void render(TextView view, String raw, AppCatalogClient.Entry entry) {
        String markdown = raw == null || raw.trim().isEmpty()
                ? "No description available." : raw;
        // Bound exceptionally large third-party READMEs before UI parsing.
        if (markdown.length() > 60_000) markdown = markdown.substring(0, 60_000) + "\n\n…";
        markwon.setMarkdown(view, resolveRelativeLinks(markdown, entry));
    }
    /** Resolve common repository-relative README images and links for Markwon. */
    static String resolveRelativeLinks(String markdown, AppCatalogClient.Entry entry) {
        if (markdown == null || entry == null) return markdown;
        String origin = entry.descriptionFileUrl != null ? entry.descriptionFileUrl
                : entry.descriptionFallbackUrl;
        if (origin == null || !origin.startsWith("https://raw.githubusercontent.com/"))
            return markdown;
        Matcher matcher = LINK.matcher(markdown);
        StringBuffer result = new StringBuffer();
        while (matcher.find()) {
            String dest = matcher.group(2);
            String target = dest;
            // URLs, anchor references, and data: URIs must never be rewritten.
            if (dest.matches("[A-Za-z0-9_./%+~#-]+") && !dest.startsWith("#")
                    && !dest.startsWith("/") && !dest.contains("\\")) {
                try {
                    URI resolved = URI.create(origin).resolve(dest).normalize();
                    if ("https".equals(resolved.getScheme())
                            && "raw.githubusercontent.com".equals(resolved.getHost()))
                        target = resolved.toASCIIString();
                } catch (IllegalArgumentException ignored) { }
            }
            matcher.appendReplacement(result,
                    Matcher.quoteReplacement(matcher.group(1) + "(" + target + ")"));
        }
        matcher.appendTail(result);
        return result.toString();
    }
}

package yandex.cloud.dashboard.integration.common;

import org.apache.http.HttpEntity;
import org.apache.http.HttpResponse;
import org.apache.http.StatusLine;
import org.apache.http.client.ResponseHandler;
import org.apache.http.util.EntityUtils;

import java.io.IOException;

/**
 * @author akirakozov
 */
public class DefaultResponseHandler implements ResponseHandler<String> {

    @Override
    public String handleResponse(final HttpResponse response) throws IOException {
        final StatusLine statusLine = response.getStatusLine();
        final HttpEntity entity = response.getEntity();
        if (statusLine.getStatusCode() >= 300) {
            throw new HttpResponseException(statusLine.getStatusCode(),
                    statusLine.getReasonPhrase(), entityToString(entity));
        }
        return entityToString(entity);
    }

    private String entityToString(HttpEntity entity) throws IOException {
        return entity == null ? null : EntityUtils.toString(entity);
    }

}

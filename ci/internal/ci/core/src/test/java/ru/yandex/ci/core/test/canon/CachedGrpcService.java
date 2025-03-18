package ru.yandex.ci.core.test.canon;

import java.io.IOException;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.lang.reflect.ParameterizedType;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.text.DecimalFormat;
import java.util.Iterator;
import java.util.List;
import java.util.stream.Collectors;

import javax.annotation.Nullable;

import com.google.common.base.Preconditions;
import com.google.common.hash.Hashing;
import com.google.gson.Gson;
import com.google.gson.GsonBuilder;
import com.google.protobuf.GeneratedMessageV3;
import com.google.protobuf.util.JsonFormat;
import io.grpc.BindableService;
import io.grpc.StatusRuntimeException;
import io.grpc.stub.AbstractBlockingStub;
import io.grpc.stub.ServerCallStreamObserver;
import io.grpc.stub.StreamObserver;
import javassist.util.proxy.MethodHandler;
import javassist.util.proxy.ProxyFactory;
import one.util.streamex.StreamEx;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.util.FileSystemUtils;

import ru.yandex.ci.core.proto.ProtobufReflectionUtils;
import ru.yandex.ci.core.proto.ProtobufSerialization;

/**
 * Кеширующий grpc сервер. Может работать в двух режимах.
 * Режим канонизации - запрос отправляется на реальный сервер, ответ сохраняется локально и отдается клиенту.
 * Обычный режим - результаты берутся из локальных файлов, удаленный сервер при этом не вызывается.
 */
public class CachedGrpcService implements MethodHandler {
    private static final Logger log = LoggerFactory.getLogger(CachedGrpcService.class);

    private static final String REQUEST_FILE_NAME = "request.json";
    private static final String RESPONSE_FILE_NAME = "response.json";
    private static final String RESPONSE_EXCEPTION_FILE_NAME = "exception.json";
    private static final String STREAM_FILES_PATTERN = "0000'.json'";

    private final AbstractBlockingStub<?> realService;
    private final Path dataPath;
    private final boolean canonize;

    public CachedGrpcService(AbstractBlockingStub<?> realService, Path dataPath, boolean canonize) {
        this.realService = realService;
        this.dataPath = dataPath;
        this.canonize = canonize;
    }

    public static <T extends BindableService> T makeProxy(
            AbstractBlockingStub<?> realServiceStub,
            Class<T> implBase,
            Path dataPath,
            boolean canonize
    )
            throws ReflectiveOperationException {

        ProxyFactory factory = new ProxyFactory();
        factory.setSuperclass(implBase);

        CachedGrpcService methodHandler = new CachedGrpcService(realServiceStub, dataPath, canonize);
        //noinspection unchecked
        return (T) factory.create(new Class<?>[0], new Object[0], methodHandler);
    }

    @Nullable
    @Override
    @SuppressWarnings({"rawtypes", "unchecked"})
    public Object invoke(Object self, Method thisMethod, Method proceed, Object[] args) throws Throwable {
        GeneratedMessageV3 request = (GeneratedMessageV3) args[0];
        Path canonPath = createRequestPath((BindableService) self, thisMethod.getName(), request);

        StreamObserver responseObserver = (StreamObserver) args[1];
        Method realMethod = realService.getClass().getMethod(thisMethod.getName(), request.getClass());
        boolean isStreamingResponse = isStreamingResponse(realMethod);

        if (canonize) {
            log.info("Canonize real server response for method {} to {}", thisMethod.getName(), canonPath);

            FileSystemUtils.deleteRecursively(canonPath);
            Files.createDirectories(canonPath);

            dumpMessage(canonPath, request, REQUEST_FILE_NAME);

            try {
                Object response = realMethod.invoke(realService, request);

                if (isStreamingResponse) {
                    canonizeStream(canonPath, (Iterator<? extends GeneratedMessageV3>) response, responseObserver);
                } else {
                    dumpMessage(canonPath, (GeneratedMessageV3) response, RESPONSE_FILE_NAME);
                    responseObserver.onNext(response);
                }

                responseObserver.onCompleted();

            } catch (InvocationTargetException e) {
                Throwable cause = e.getCause();
                if (!(cause instanceof StatusRuntimeException exception)) {
                    throw cause;
                }

                dumpException(canonPath, exception);
                responseObserver.onError(exception);
            } catch (StatusRuntimeException e) {
                dumpException(canonPath, e);
                responseObserver.onError(e);
            }

        } else {

            Preconditions.checkState(
                    Files.exists(canonPath),
                    "directory %s doesn't exist. Maybe request was changed, try canonize data. Request: \n%s",
                    canonPath, ProtobufSerialization.serializeToJsonString(request)
            );
            Preconditions.checkState(Files.isDirectory(canonPath), "%s is not a directory", canonPath);

            Path exceptionPath = canonPath.resolve(RESPONSE_EXCEPTION_FILE_NAME);
            if (Files.exists(exceptionPath)) {
                log.info("Load exceptional response for {} from {}", thisMethod.getName(), canonPath);
                String json = Files.readString(exceptionPath);
                StatusRuntimeException exception = new Gson().fromJson(json, StatusRuntimeException.class);
                responseObserver.onError(exception);

            } else {
                List<Path> canonFilePaths;
                try (var directories = Files.newDirectoryStream(canonPath)) {
                    canonFilePaths = StreamEx.of(directories.spliterator())
                            .sorted()
                            .remove(path -> path.getFileName().endsWith(REQUEST_FILE_NAME))
                            .collect(Collectors.toList());
                }

                log.info("Load response for {} from {}, {} files",
                        thisMethod.getName(), canonPath, canonFilePaths.size()
                );


                GeneratedMessageV3.Builder builder;
                if (isStreamingResponse) {
                    ParameterizedType returnType = (ParameterizedType) realMethod.getGenericReturnType();
                    Class<?> iteratorItemClass = (Class<?>) returnType.getActualTypeArguments()[0];
                    builder = ProtobufReflectionUtils.getMessageBuilder(iteratorItemClass);
                } else {
                    builder = ProtobufReflectionUtils.getMessageBuilder(realMethod.getReturnType());
                }
                for (Path canonFilePath : canonFilePaths) {
                    builder.clear();
                    JsonFormat.parser().ignoringUnknownFields().merge(
                            Files.newBufferedReader(canonFilePath),
                            builder
                    );
                    responseObserver.onNext(builder.build());
                }

                responseObserver.onCompleted();
            }
        }

        return null;
    }

    private boolean isStreamingResponse(Method realMethod) {
        return Iterator.class.isAssignableFrom(realMethod.getReturnType());
    }

    @SuppressWarnings({"rawtypes", "unchecked"})
    private void canonizeStream(
            Path canonPath, Iterator<? extends GeneratedMessageV3> response,
            StreamObserver responseObserver
    ) throws IOException {

        DecimalFormat decimalFormat = new DecimalFormat(STREAM_FILES_PATTERN);
        ServerCallStreamObserver streamObserver = (ServerCallStreamObserver) responseObserver;
        int num = 0;
        while (response.hasNext()) {
            GeneratedMessageV3 value = response.next();
            dumpMessage(canonPath, value, decimalFormat.format(num));
            num++;

            streamObserver.onNext(value);
            // стриминг прекращен на стороне клиента
            if (streamObserver.isCancelled()) {
                return;
            }
        }
    }

    private void dumpMessage(Path canonPath, GeneratedMessageV3 value, String fileName) throws IOException {
        Files.write(
                canonPath.resolve(fileName),
                List.of(ProtobufSerialization.serializeToJsonString(value)),
                StandardCharsets.UTF_8
        );
    }

    private void dumpException(Path canonPath, StatusRuntimeException exception) throws IOException {
        Gson gson = new GsonBuilder().setPrettyPrinting().create();
        String json = gson.toJson(exception);
        Files.writeString(canonPath.resolve(RESPONSE_EXCEPTION_FILE_NAME), json);
    }

    private Path createRequestPath(
            BindableService proxyService,
            String method,
            GeneratedMessageV3 request
    ) {
        byte[] requestBytes = request.toByteArray();
        String requestHash = getRequestHash(requestBytes);

        return dataPath
                .resolve(proxyService.getClass().getSuperclass().getEnclosingClass().getSimpleName())
                .resolve(method)
                .resolve(requestHash);
    }

    private String getRequestHash(byte[] requestBytes) {
        return Hashing.sha1().hashBytes(requestBytes).toString();
    }
}

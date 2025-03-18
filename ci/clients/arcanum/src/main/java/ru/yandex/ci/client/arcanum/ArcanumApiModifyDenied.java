package ru.yandex.ci.client.arcanum;

import java.util.List;

import retrofit2.Response;

class ArcanumApiModifyDenied implements ArcanumClientImpl.ArcanumApiModify {

    static final ArcanumClientImpl.ArcanumApiModify INSTANCE = new ArcanumApiModifyDenied();

    private ArcanumApiModifyDenied() {
        //
    }

    @Override
    public String restartCheck(String id) {
        throw unsupportedOperationException();
    }

    @Override
    public Response<Void> setMergeRequirement(long reviewRequestId, ArcanumMergeRequirementDto request) {
        throw unsupportedOperationException();
    }

    @Override
    public Response<Void> setMergeRequirementStatus(long reviewRequestId, long diffSetId,
                                                    UpdateCheckStatusRequest request) {
        throw unsupportedOperationException();
    }

    @Override
    public Response<Void> createReviewRequestComment(long reviewRequestId,
                                                     ArcanumClientImpl.ReviewRequestComment comment) {
        throw unsupportedOperationException();
    }

    @Override
    public Response<Void> setMergeRequirementsWithCheckingActiveDiffSet(
            long reviewRequestId,
            long diffSetId,
            List<UpdateCheckRequirementRequestDto> requirements
    ) {
        throw unsupportedOperationException();
    }

    @Override
    public Response<Void> registerCiCheck(long diffSetId, RegisterCheckRequestDto registerCheckRequest) {
        throw unsupportedOperationException();
    }

    private static RuntimeException unsupportedOperationException() {
        return new UnsupportedOperationException("Method is not supported in ArcanumApiReadOnly");
    }
}

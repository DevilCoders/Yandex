package ru.yandex.ci.flow.engine.runtime.di.model;

import java.util.List;
import java.util.UUID;

import com.fasterxml.jackson.annotation.JsonCreator;
import com.google.gson.JsonObject;
import lombok.Getter;
import lombok.ToString;

import ru.yandex.ci.core.job.JobResourceType;
import ru.yandex.ci.core.resources.Resource;
import ru.yandex.ci.flow.engine.model.FlowLaunchId;
import ru.yandex.ci.util.gson.CiGson;

@Getter
@ToString
public class StoredResource extends StoredSourceCodeObject<Resource> {
    private final StoredResourceId id;

    private final String flowId;

    private final FlowLaunchId flowLaunchId;

    private final JobResourceType resourceType;

    private final List<StoredResourceModification> modifications;

    private transient Resource instance;

    private transient boolean instantiated;

    @JsonCreator
    public StoredResource(StoredResourceId id,
                          String flowId,
                          FlowLaunchId flowLaunchId,
                          UUID sourceCodeId,
                          JsonObject object,
                          JobResourceType resourceType,
                          List<StoredResourceModification> modifications) {
        super(sourceCodeId, object);
        this.id = id;
        this.flowId = flowId;
        this.flowLaunchId = flowLaunchId;
        this.resourceType = resourceType;
        this.modifications = modifications;
    }

    public ResourceRef toRef() {
        return new ResourceRef(id, resourceType, getSourceCodeId());
    }

    public JobResourceType getResourceType() {
        return resourceType;
    }

    public StoredResourceId getId() {
        return id;
    }

    public String getFlowId() {
        return flowId;
    }

    public FlowLaunchId getFlowLaunchId() {
        return flowLaunchId;
    }

    public List<StoredResourceModification> getModifications() {
        return modifications == null ? List.of() : modifications;
    }

    public Resource instantiate() {
        if (instantiated) {
            return instance;
        }

        var object = getObject();
        if (object == null) {
            return instance;
        }

        try {
            instance = CiGson.instance().fromJson(object, Resource.class);
            this.instantiated = true;
            return instance;
        } catch (RuntimeException e) {
            throw new RuntimeException("Unable to instantiate resource: " + resourceType, e);
        }
    }

}

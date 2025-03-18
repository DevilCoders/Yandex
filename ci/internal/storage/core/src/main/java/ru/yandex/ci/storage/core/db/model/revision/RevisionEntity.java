package ru.yandex.ci.storage.core.db.model.revision;

import java.time.Instant;

import lombok.AllArgsConstructor;
import lombok.Value;

import yandex.cloud.binding.schema.Column;
import yandex.cloud.binding.schema.Table;
import yandex.cloud.repository.db.Entity;
import yandex.cloud.repository.kikimr.DbType;

import ru.yandex.ci.core.arc.ArcCommit;

@Value
@AllArgsConstructor
@Table(name = "Revisions")
public class RevisionEntity implements Entity<RevisionEntity> {
    Id id;

    String commitId;
    String author;
    String message;

    @Column(dbType = DbType.TIMESTAMP)
    Instant created;

    public RevisionEntity(String branch, ArcCommit commit) {
        this.id = new Id(commit.getSvnRevision(), branch);
        this.commitId = commit.getRevision().getCommitId();
        this.author = commit.getAuthor();
        this.message = commit.getMessage();
        this.created = commit.getCreateTime();
    }

    public RevisionEntity(String branch, long revision) {
        this.id = new Id(revision, branch);
        this.commitId = "-";
        this.author = "-";
        this.message = "-";
        this.created = Instant.now();
    }

    @Override
    public Id getId() {
        return this.id;
    }

    @Value
    public static class Id implements Entity.Id<RevisionEntity> {
        @Column(dbType = DbType.UINT64)
        Long number;

        String branch;

        @Override
        public String toString() {
            return '[' + branch + '/' + number + ']';
        }
    }
}

# Set and release locks

Autoduty uses mlock to set and release locks on containers AND dom0s.
* It waits indefinitely if lock is acquired by someone else if intention is to acquire lock.
* It ignores that the very lock was released by someone if intention is to release it.

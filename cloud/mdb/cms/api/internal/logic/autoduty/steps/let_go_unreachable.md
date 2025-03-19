# Is dom0 unreachable?

This step is multipurpose. It's needed when:
* during "analyze" stage autoduty should skip steps after this one, because it's impossible to perform analysis. No point to analyze DOM0 if autoduty cannot connect to it. But as a last effort autoduty waits a little, because some times DOM0 becomes reachable and Wall-e deletes the request.
* during "let go" autoduty does not wait, but immediately gives away the DOM0, skipping next steps.

The step uses Juggler API. If Active Juggler checks show that DOM0 has CRITs for some time, it considers DOM0 unreachable.
If according to Juggler DOM0 is reachable, Autoduty takes timeout, so that Wall-e could understand it.
If time is up and the request is not deleted, autoduty gives away dom0.
Even if it's "OK" in Juggler. Wall-e in this case knows best what it intended to do.

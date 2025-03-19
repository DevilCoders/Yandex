# Common information about shipments

In production CMS considers everything not under current production deploy api as a testing. CMS does not care about testing machines.
You will see them in `skipped` section.

If shipments fail you can try and finish them with your hands. But for now you have no mechanism in CMS to ensure that after your actions the state is desired and continue execution.
So you will have to manually continue execution by performing SQL requests in CMS DB.
See
  * https://st.yandex-team.ru/MDB-9634
  * https://st.yandex-team.ru/MDB-9691
After those tickets CMS
will look in shipments with problems and if they are OK, continue automatically
or CMS will give you an opportunity to skip certain problematic step and continue execution.

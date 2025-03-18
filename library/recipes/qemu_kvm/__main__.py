import library.python.testing.recipe
import library.recipes.qemu_kvm.lib as qemu_kvm_lib


if __name__ == "__main__":
    library.python.testing.recipe.declare_recipe(qemu_kvm_lib.start, qemu_kvm_lib.stop)

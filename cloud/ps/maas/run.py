from clients import instances
from supdater import graphs

def main():
    '''
    x = instances.Instances()
    r = x.getComputeInstancesFromFolder('b1gnscs6kkq2bj7um14s')
    print(r)
    s = graphs.Graphs()
    z = s.getGraphInstances('Mindbox_vrouter_flow')
    if set(r) == set(z):
        print('Solomon data up to date!')
    print(z)
    w = s.changeGraphInstances('Mindbox_vrouter_flow', r)
    print(w)
    '''
    a = graphs.Graphs()
    b = a.createNewGraphFromTemplate('cpu', ['fhmif8ke2ae9ckqf1r9i'], 'test')
    print(b)
    return 0

if __name__ == '__main__':
    main()

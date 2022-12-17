from django.urls import path
from .import views

urlpatterns = [
    
    # path('', views.index, name='index'),
    # path('map/', views.map, name='map'),
    # path('use/', views.user, name='user')

    path('', views.map, name='map'),
    path('use/', views.user, name='user'),
    path('about/', views.index, name='index')
    # path(')
]